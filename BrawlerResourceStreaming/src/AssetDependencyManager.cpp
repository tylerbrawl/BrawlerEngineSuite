module;
#include <vector>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <algorithm>
#include <thread>
#include <array>
#include <ranges>
#include <cassert>

module Brawler.AssetDependencyManager;
import Brawler.AssetManager;
import Brawler.JobSystem;
import Util.Math;
import Brawler.AssetDataContexts;
import Brawler.I_Asset;

namespace
{
	static const std::uint32_t LOGICAL_CPU_CORE_COUNT = std::thread::hardware_concurrency();

	// The following functions are used to determine how much data is loaded or unloaded in
	// each asset data load/unload job.
	// 
	// Experimentation will almost certainly be required to find a good balance between
	// loading times and stutter reduction.
	
	constexpr std::size_t GetAssetDataLoadLimit(const Brawler::AssetDependencyManager::OptimizationMode optimizationMode)
	{
		using OptimizationMode = Brawler::AssetDependencyManager::OptimizationMode;

		switch (optimizationMode)
		{
		case OptimizationMode::REDUCE_STUTTER:
			return Util::Math::MegabytesToBytes(8);

		case OptimizationMode::REDUCE_LOAD_TIMES:
			return Util::Math::MegabytesToBytes(128);

		default:
		{
			assert(false && "ERROR: An unspecified AssetDependencyManager::OptimizationMode was provided for GetAssetDataLoadLimit()!");
			return Util::Math::MegabytesToBytes(8);
		}
		}
	}

	constexpr std::size_t GetAssetDataUnloadLimit(const Brawler::AssetDependencyManager::OptimizationMode optimizationMode)
	{
		using OptimizationMode = Brawler::AssetDependencyManager::OptimizationMode;

		switch (optimizationMode)
		{
		case OptimizationMode::REDUCE_STUTTER:
			return Util::Math::MegabytesToBytes(16);

		case OptimizationMode::REDUCE_LOAD_TIMES:
			return Util::Math::MegabytesToBytes(256);

		default:
		{
			assert(false && "ERROR: An unspecified AssetDependencyManager::OptimizationMode was provided for GetAssetDataUnloadLimit()!");
			return Util::Math::MegabytesToBytes(16);
		}
		}
	}

	void ExecuteAssetDataLoadRequests(std::vector<std::unique_ptr<Brawler::AssetDataLoadRequest>>& loadRequestArr)
	{
		for (auto& requestPtr : loadRequestArr)
		{
			Brawler::AssetDataLoadContext context{
				.NumBytesToLoad = requestPtr->NumBytesToLoad
			};

			requestPtr->Asset.ExecuteAssetDataLoad(context);
		}
	}

	void ExecuteAssetDataUnloadRequests(std::vector<std::unique_ptr<Brawler::AssetDataUnloadRequest>>& unloadRequestArr)
	{
		for (auto& requestPtr : unloadRequestArr)
		{
			Brawler::AssetDataUnloadContext context{
				.NumBytesToUnload = requestPtr->NumBytesToUnload
			};

			requestPtr->Asset.ExecuteAssetDataUnload(context);
		}
	}
}

namespace IMPL
{
	template <>
	void AssetDataRequestHandler::PullAssetDataRequests<Brawler::AssetDataLoadRequest>(AssetDataRequestPullContext<Brawler::AssetDataLoadRequest>& pullContext)
	{
		while (!mLoadRequestQueue.empty())
		{
			// We sort our queues in reverse order to make use of the std::vector::pop_back() API.
			std::unique_ptr<Brawler::AssetDataLoadRequest>& pendingRequest{ mLoadRequestQueue.back() };

			if ((pullContext.BytesHandledInRequests + pendingRequest->NumBytesToLoad) <= pullContext.MaxDesiredBytesAllowed)
			{
				pullContext.BytesHandledInRequests += pendingRequest->NumBytesToLoad;

				pullContext.RequestArr.push_back(std::move(pendingRequest));
				mLoadRequestQueue.pop_back();
			}
			else
				break;
		}

		// If all of the requests in this queue are above the limit, then pull just one request.
		// Otherwise, no requests will ever be fulfilled.
		if (pullContext.RequestArr.empty() && !mLoadRequestQueue.empty())
		{
			pullContext.BytesHandledInRequests += mLoadRequestQueue.back()->NumBytesToLoad;

			pullContext.RequestArr.push_back(std::move(mLoadRequestQueue.back()));
			mLoadRequestQueue.pop_back();
		}
	}

	template <>
	void AssetDataRequestHandler::PullAssetDataRequests<Brawler::AssetDataUnloadRequest>(AssetDataRequestPullContext<Brawler::AssetDataUnloadRequest>& pullContext)
	{
		while (!mUnloadRequestQueue.empty())
		{
			// We sort our queues in reverse order to make use of the std::vector::pop_back() API.
			std::unique_ptr<Brawler::AssetDataUnloadRequest>& pendingRequest{ mUnloadRequestQueue.back() };

			if ((pullContext.BytesHandledInRequests + pendingRequest->NumBytesToUnload) <= pullContext.MaxDesiredBytesAllowed)
			{
				pullContext.BytesHandledInRequests += pendingRequest->NumBytesToUnload;

				pullContext.RequestArr.push_back(std::move(pendingRequest));
				mUnloadRequestQueue.pop_back();
			}
			else
				break;
		}

		// If all of the requests in this queue are above the limit, then pull just one request.
		// Otherwise, no requests will ever be fulfilled.
		if (pullContext.RequestArr.empty() && !mUnloadRequestQueue.empty())
		{
			pullContext.BytesHandledInRequests += mUnloadRequestQueue.back()->NumBytesToUnload;

			pullContext.RequestArr.push_back(std::move(mUnloadRequestQueue.back()));
			mUnloadRequestQueue.pop_back();
		}
	}

	AssetDataRequestHandler::AssetDataRequestHandler() :
		mLoadRequestQueue(),
		mLoadRequestQueueCritSection(),
		mUnloadRequestQueue(),
		mUnloadRequestQueueCritSection()
	{}

	void AssetDataRequestHandler::SubmitAssetDataLoadRequest(std::unique_ptr<Brawler::AssetDataLoadRequest>&& loadRequest)
	{
		std::scoped_lock<std::mutex> lock{ mLoadRequestQueueCritSection };
		mLoadRequestQueue.push_back(std::move(loadRequest));
	}

	void AssetDataRequestHandler::SubmitAssetDataUnloadRequest(std::unique_ptr<Brawler::AssetDataUnloadRequest>&& unloadRequest)
	{
		std::scoped_lock<std::mutex> lock{ mUnloadRequestQueueCritSection };
		mUnloadRequestQueue.push_back(std::move(unloadRequest));
	}

	void AssetDataRequestHandler::SortAssetDataRequestQueues()
	{
		// Sort the load and unload requests in order of decreasing size. This should theoretically
		// increase throughput.
		//
		// In actuality, we retrieve the requests in the opposite order; we only sort it like this
		// so that we can quickly remove elements from the queue using std::vector::pop_back().
		std::ranges::sort(mLoadRequestQueue, [] (const std::unique_ptr<Brawler::AssetDataLoadRequest>& lhs, const std::unique_ptr<Brawler::AssetDataLoadRequest>& rhs)
			{
				return (lhs->NumBytesToLoad > rhs->NumBytesToLoad);
			});

		std::ranges::sort(mUnloadRequestQueue, [] (const std::unique_ptr<Brawler::AssetDataUnloadRequest>& lhs, const std::unique_ptr<Brawler::AssetDataUnloadRequest>& rhs)
			{
				return (lhs->NumBytesToUnload > rhs->NumBytesToUnload);
			});
	}

	bool AssetDataRequestHandler::IsLoadRequestQueueEmpty() const
	{
		return mLoadRequestQueue.empty();
	}

	bool AssetDataRequestHandler::IsUnloadRequestQueueEmpty() const
	{
		return mUnloadRequestQueue.empty();
	}
}

namespace Brawler
{
	AssetDependencyManager::AssetDependencyManager(AssetManager& owningManager) :
		mOwningManager(&owningManager),
		mAssetDependencyGroups(),
		mPersistentDependencyGroups(),
		mRequestHandlerArr(),
		mOptimizationMode(OptimizationMode::REDUCE_STUTTER)
	{}

	void AssetDependencyManager::UpdateAssetDependencies()
	{
		// First, activate the pending asset dependency groups.
		ActivatePendingAssetDependencies();

		// Next, check the dependencies for asset data update requests.
		CheckDependenciesForAssetDataUpdateRequests();

		// Finally, process *SOME* of these requests for this update. By limiting the number of
		// processed requests, we can help to reduce stuttering during gameplay.
		ProcessAssetDataUpdateRequests();
	}

	void AssetDependencyManager::SubmitAssetDependencyGroup(std::unique_ptr<AssetDependencyGroup>&& dependencyGroup)
	{
		std::scoped_lock<std::mutex> lock{ mAssetDependencyGroups.PendingGroupsCritSection };
		mAssetDependencyGroups.PendingGroups.push_back(std::move(dependencyGroup));
	}

	void AssetDependencyManager::SubmitAssetDependencyGroup(std::unique_ptr<PersistentAssetDependencyGroup>&& persistentGroup)
	{
		std::scoped_lock<std::mutex> lock{ mPersistentDependencyGroups.PendingGroupsCritSection };
		mPersistentDependencyGroups.PendingGroups.push_back(std::move(persistentGroup));
	}

	void AssetDependencyManager::SubmitAssetDataLoadRequest(std::unique_ptr<AssetDataLoadRequest>&& loadRequest, const JobPriority priority)
	{
		mRequestHandlerArr[std::to_underlying(priority)].SubmitAssetDataLoadRequest(std::move(loadRequest));
	}

	void AssetDependencyManager::SubmitAssetDataUnloadRequest(std::unique_ptr<AssetDataUnloadRequest>&& unloadRequest, const JobPriority priority)
	{
		mRequestHandlerArr[std::to_underlying(priority)].SubmitAssetDataUnloadRequest(std::move(unloadRequest));
	}

	void AssetDependencyManager::SetOptimizationMode(const OptimizationMode optimizationMode)
	{
		mOptimizationMode = optimizationMode;
	}

	void AssetDependencyManager::ActivatePendingAssetDependencies()
	{
		// No other threads should be writing to the pending groups at this point.

		for (auto itr = mAssetDependencyGroups.PendingGroups.begin(); itr != mAssetDependencyGroups.PendingGroups.end();)
		{
			mAssetDependencyGroups.ActiveGroups.push_back(std::move(*itr));
			itr = mAssetDependencyGroups.PendingGroups.erase(itr);
		}

		for (auto itr = mPersistentDependencyGroups.PendingGroups.begin(); itr != mPersistentDependencyGroups.PendingGroups.end();)
		{
			mPersistentDependencyGroups.ActiveGroups.push_back(std::move(*itr));
			itr = mPersistentDependencyGroups.PendingGroups.erase(itr);
		}
	}

	void AssetDependencyManager::CheckDependenciesForAssetDataUpdateRequests()
	{
		// At every node of every asset dependency graph, check the I_Assets present
		// for update requests, and spawn a new job for each child group. This may
		// or may not prove to be an optimal way of going about this.

		Brawler::JobGroup requestCheckGroup{};
		requestCheckGroup.Reserve(mAssetDependencyGroups.ActiveGroups.size() + mPersistentDependencyGroups.ActiveGroups.size());

		for (auto& activeGroup : mAssetDependencyGroups.ActiveGroups)
		{
			AssetDependencyGroup* const groupPtr = activeGroup.get();
			requestCheckGroup.AddJob([groupPtr] ()
			{
				groupPtr->CheckDependenciesForAssetDataRequests();
			});
		}

		for (auto& activePersistentGroup : mPersistentDependencyGroups.ActiveGroups)
		{
			PersistentAssetDependencyGroup* const groupPtr = activePersistentGroup.get();
			requestCheckGroup.AddJob([groupPtr] ()
			{
				groupPtr->CheckDependenciesForAssetDataRequests();
			});
		}

		requestCheckGroup.ExecuteJobs();
	}

	void AssetDependencyManager::ProcessAssetDataUpdateRequests()
	{
		// Sort all of the asset request queues. We do this via jobs because sorting can
		// take quite some time, and sorting the individual request handlers does not share
		// any data.
		{
			Brawler::JobGroup requestQueueFlushGroup{};
			requestQueueFlushGroup.Reserve(mRequestHandlerArr.size());

			for (auto& requestHandler : mRequestHandlerArr)
				requestQueueFlushGroup.AddJob([&requestHandler] ()
				{
					requestHandler.SortAssetDataRequestQueues();
				});

			requestQueueFlushGroup.ExecuteJobs();
		}

		// Now, we create jobs for loading and unloading asset data. We want to make sure that
		// all requests with JobPriority::CRITICAL priority are fulfilled before we exit the
		// asset dependency update.
		{
			static const std::uint32_t JOB_COUNT_LIMIT = LOGICAL_CPU_CORE_COUNT;
			Brawler::JobGroup assetDataRequestGroup{};

			for (std::uint32_t i = 0; i < JOB_COUNT_LIMIT; ++i)
			{
				std::shared_ptr<std::vector<std::unique_ptr<AssetDataLoadRequest>>> pulledLoadRequests{ std::make_shared<std::vector<std::unique_ptr<AssetDataLoadRequest>>>(GetLoadRequestsForSingleJob()) };
				if (!pulledLoadRequests->empty())
				{
					assetDataRequestGroup.AddJob([pulledLoadRequests]() mutable
					{
						ExecuteAssetDataLoadRequests(*pulledLoadRequests);
					});
				}

				std::shared_ptr<std::vector<std::unique_ptr<AssetDataUnloadRequest>>> pulledUnloadRequests{ std::make_shared<std::vector<std::unique_ptr<AssetDataUnloadRequest>>>(GetUnloadRequestsForSingleJob()) };
				if (!pulledUnloadRequests->empty())
				{
					assetDataRequestGroup.AddJob([pulledUnloadRequests]() mutable
					{
						ExecuteAssetDataUnloadRequests(*pulledUnloadRequests);
					});
				}
			}

			// Fulfill all requests with JobPriority::CRITICAL priority.

			while (!mRequestHandlerArr[std::to_underlying(JobPriority::CRITICAL)].IsLoadRequestQueueEmpty())
			{
				std::shared_ptr<std::vector<std::unique_ptr<AssetDataLoadRequest>>> criticalLoadRequests{ std::make_shared<std::vector<std::unique_ptr<AssetDataLoadRequest>>>(GetLoadRequestsForSingleJob()) };
				
				assetDataRequestGroup.AddJob([criticalLoadRequests]() mutable
				{
					ExecuteAssetDataLoadRequests(*criticalLoadRequests);
				});
			}

			while (!mRequestHandlerArr[std::to_underlying(JobPriority::CRITICAL)].IsUnloadRequestQueueEmpty())
			{
				std::shared_ptr<std::vector<std::unique_ptr<AssetDataUnloadRequest>>> criticalUnloadRequests{ std::make_shared<std::vector<std::unique_ptr<AssetDataUnloadRequest>>>(GetUnloadRequestsForSingleJob()) };

				assetDataRequestGroup.AddJob([criticalUnloadRequests]() mutable
				{
					ExecuteAssetDataUnloadRequests(*criticalUnloadRequests);
				});
			}

			assetDataRequestGroup.ExecuteJobs();
		}
	}

	std::vector<std::unique_ptr<AssetDataLoadRequest>> AssetDependencyManager::GetLoadRequestsForSingleJob()
	{
		// Go through the queues in order of decreasing priority and pull as many requests as we can from
		// each.
		IMPL::AssetDataRequestPullContext<AssetDataLoadRequest> loadRequestPullContext{
			.RequestArr{},
			.BytesHandledInRequests = 0,
			.MaxDesiredBytesAllowed = GetAssetDataLoadLimit(mOptimizationMode)
		};

		for (auto& requestHandler : mRequestHandlerArr | std::views::reverse)
			requestHandler.PullAssetDataRequests<AssetDataLoadRequest>(loadRequestPullContext);

		return std::move(loadRequestPullContext.RequestArr);
	}

	std::vector<std::unique_ptr<AssetDataUnloadRequest>> AssetDependencyManager::GetUnloadRequestsForSingleJob()
	{
		// Go through the queues in order of decreasing priority and pull as many requests as we can from
		// each.
		IMPL::AssetDataRequestPullContext<AssetDataUnloadRequest> unloadRequestPullContext{
			.RequestArr{},
			.BytesHandledInRequests = 0,
			.MaxDesiredBytesAllowed = GetAssetDataUnloadLimit(mOptimizationMode)
		};

		for (auto& requestHandler : mRequestHandlerArr | std::views::reverse)
			requestHandler.PullAssetDataRequests<AssetDataUnloadRequest>(unloadRequestPullContext);

		return std::move(unloadRequestPullContext.RequestArr);
	}
}