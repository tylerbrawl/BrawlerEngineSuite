module;
#include <span>
#include <vector>
#include <unordered_map>
#include <cassert>
#include <optional>
#include <thread>
#include <ranges>
#include "DxDef.h"

module Brawler.D3D12.FrameGraphExecutionContext;
import Brawler.JobSystem;
import Brawler.D3D12.FrameGraphBuilder;
import Brawler.SortedVector;
import Brawler.CompositeEnum;
import Brawler.D3D12.RenderPassBundle;
import Util.D3D12;
import Util.Math;
import Brawler.D3D12.RenderPass;
import Util.Engine;
import Brawler.D3D12.GPUCommandManager;
import Brawler.D3D12.GPUExecutionModuleRecordContext;

namespace
{
	/// <summary>
	/// This is the maximum number of RenderPass instances which can be assigned to
	/// a single GPUExecutionModule.
	/// 
	/// Each GPUExecutionModule represents a call to ID3D12CommandQueue::ExecuteCommandLists().
	/// Finding the right value for this variable will require experimentation.
	/// 
	/// Note, however, that regardless of what this value is set to, RenderPassBundles
	/// only have their RenderPasses combined into a single GPUExecutionModule if
	/// all of their RenderPasses are direct ones, and not compute or copy. 
	/// 
	/// In addition, if a RenderPassBundle contains more RenderPasses than this value, 
	/// then its RenderPasses will still be placed into a single GPUExecutionModule. 
	/// However, it is guaranteed that no other RenderPasses will also be added to said
	/// module from other RenderPassBundles.
	/// </summary>
	static constexpr std::size_t MAX_RENDER_PASSES_PER_GPU_EXECUTION_MODULE = 200;
}

namespace Brawler
{
	namespace D3D12
	{
		void FrameGraphExecutionContext::CompileFrameGraph(const std::span<FrameGraphBuilder> builderSpan, TransientGPUResourceAliasTracker&& aliasTracker)
		{
			mAliasTracker = std::move(aliasTracker);
			
			CreateGPUExecutionModules(builderSpan);

			if (mExecutionModuleArr.size() > 0) [[likely]]
				PerformGPUResourceAnalysis();
		}

		void FrameGraphExecutionContext::SubmitFrameGraph(FrameGraphFenceCollection& fenceCollection)
		{
			// First, assign each GPUExecutionModule a unique ID for this frame. They will
			// use this to know where they should submit their command lists to later on.
			{
				std::uint32_t currModuleID = 0;

				for (auto& executionModule : mExecutionModuleArr)
					executionModule.SetModuleID(currModuleID++);
			}
			
			// Next, we inform the GPUCommandManager that we are going to begin submitting
			// GPUCommandContext instances for execution on the GPU. It will then initialize
			// the GPUCommandContextSink for this FrameGraph and return a std::span
			// which refers to the GPUCommandContextSubmissionPoints belonging to said sink.
			const std::span<GPUCommandContextSubmissionPoint> submitPointSpan{ Util::Engine::GetGPUCommandManager().BeginGPUCommandContextSubmission(fenceCollection, mExecutionModuleArr.size()) };

			// Now we can create a CPU job for each GPUExecutionModule to submit that
			// module's jobs to the GPU. We do this asynchronously so that the calling thread
			// can proceed to prepare for the next frame.
			if (mExecutionModuleArr.size() > 0) [[likely]]
			{
				Brawler::JobGroup executionModuleSubmitGroup{};
				executionModuleSubmitGroup.Reserve(mExecutionModuleArr.size());

				struct GPUExecutionModuleRecordInfo
				{
					GPUExecutionModule* ExecutionModulePtr;
					GPUExecutionModuleRecordContext RecordContext;
				};

				GPUExecutionModuleRecordInfo recordInfo{
					.ExecutionModulePtr = nullptr,
					.RecordContext{
						.AliasTracker = &mAliasTracker,
						.EventManager = &mEventManager,
						.SubmitPoint = nullptr
					}
				};

				for (std::size_t i = 0; i < mExecutionModuleArr.size(); ++i)
				{
					recordInfo.ExecutionModulePtr = &(mExecutionModuleArr[i]);
					recordInfo.RecordContext.SubmitPoint = &(submitPointSpan[i]);

					// We need to capture this structure by value, since we are executing these
					// jobs fully asynchronously.
					executionModuleSubmitGroup.AddJob([recordInfo] ()
					{
						recordInfo.ExecutionModulePtr->SubmitCommandListsForRenderPasses(recordInfo.RecordContext);
					});
				}

				executionModuleSubmitGroup.ExecuteJobsAsync();
			}
		}
		
		void FrameGraphExecutionContext::CreateGPUExecutionModules(const std::span<FrameGraphBuilder> builderSpan)
		{
			// First, combine the RenderPass instances contained within RenderPassBundles into
			// GPUExecutionModules as applicable.
			//
			// We impose limits on the usage of resources across queues simultaneously. (Mainly,
			// a resource used in both a direct and a compute RenderPass within the same
			// RenderPassBundle *MUST* be used in a read state for the entire RenderPassBundle.)
			//
			// For that reason, we will only combine RenderPassBundles which contain only direct
			// RenderPasses. (However, this should theoretically be the common case.)

			GPUExecutionModule currExecutionModule{};

			const auto canAddBundleToModuleLambda = [] (const GPUExecutionModule& executionModule, const RenderPassBundle& bundle) -> bool
			{
				if (executionModule.GetRenderPassCount() == 0)
					return true;

				// We need to be careful with merging sync points. If we merge a sync point into a
				// GPUExecutionModule containing only direct render passes, then any transitions it
				// defines may be made redundant by implicit resource state decay.
				if (bundle.IsSyncPoint())
					return false;

				// We only want to combine RenderPassBundles which contain only direct RenderPasses.
				if (executionModule.GetUsedQueues() != bundle.GetUsedQueues() || bundle.GetUsedQueues().CountOneBits() != 1)
					return false;

				const std::size_t moduleSizeAfterBundle = executionModule.GetRenderPassCount() + bundle.GetTotalRenderPassCount();
				return (moduleSizeAfterBundle <= MAX_RENDER_PASSES_PER_GPU_EXECUTION_MODULE);
			};

			for (auto& builder : builderSpan)
			{
				for (auto&& renderPassBundle : builder.GetRenderPassBundleSpan())
				{
					if (!canAddBundleToModuleLambda(currExecutionModule, renderPassBundle))
					{
						mExecutionModuleArr.push_back(std::move(currExecutionModule));
						currExecutionModule = GPUExecutionModule{};
					}

					currExecutionModule.AddRenderPassBundle(std::move(renderPassBundle));
				}
			}

			if (currExecutionModule.GetRenderPassCount() > 0) [[likely]]
				mExecutionModuleArr.push_back(std::move(currExecutionModule));
		}

		void FrameGraphExecutionContext::PerformGPUResourceAnalysis()
		{
			// Get a list of all of the used I_GPUResource instances in the FrameGraph which are
			// located in a D3D12_HEAP_TYPE_DEFAULT heap. We restrict the search to only this heap
			// type because resources created in either D3D12_HEAP_TYPE_UPLOAD heaps or
			// D3D12_HEAP_TYPE_READBACK heaps are never to transition out of their initial state.
			std::unordered_map<I_GPUResource*, GPUResourceEventManager> resourceEventManagerMap{};

			for (auto& executionModule : mExecutionModuleArr)
			{
				executionModule.PrepareForResourceStateTracking();
				
				const Brawler::SortedVector<I_GPUResource*> moduleResourceDependencies{ executionModule.GetResourceDependencies() };

				moduleResourceDependencies.ForEach([&resourceEventManagerMap] (I_GPUResource* const& resourceDependency)
				{
					const D3D12_HEAP_TYPE heapType{ resourceDependency->GetHeapType() };

					if (heapType == D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT)
						resourceEventManagerMap.try_emplace(resourceDependency);
				});
			}

			// Resource tracking can be a long-running task, especially since the algorithm currently
			// does it on a per-resource basis. We want to multithread this as much as possible.
			const std::uint32_t numJobsToCreate = std::thread::hardware_concurrency();
			const std::size_t numResourcesPerJob = static_cast<std::size_t>(std::ceilf(static_cast<float>(resourceEventManagerMap.size()) / static_cast<float>(numJobsToCreate)));

			Brawler::JobGroup resourceTrackingJobGroup{};
			resourceTrackingJobGroup.Reserve(numJobsToCreate);

			std::size_t numResourcesAdded = 0;

			while (numResourcesAdded < resourceEventManagerMap.size())
			{
				const std::size_t numResourcesThisJob = std::min(numResourcesPerJob, (resourceEventManagerMap.size() - numResourcesAdded));

				resourceTrackingJobGroup.AddJob([this, &resourceEventManagerMap, numResourcesThisJob, startIndex = numResourcesAdded] ()
				{
					for (auto& [resourcePtr, resourceEventManager] : resourceEventManagerMap | std::views::drop(startIndex) | std::views::take(numResourcesThisJob))
					{
						GPUResourceUsageAnalyzer resourceAnalyzer{ *resourcePtr };
						resourceAnalyzer.TraverseFrameGraph(std::span<const GPUExecutionModule>{ mExecutionModuleArr });

						resourceEventManager = resourceAnalyzer.ExtractGPUResourceEventManager();
					}
				});

				numResourcesAdded += numResourcesThisJob;
			}

			resourceTrackingJobGroup.ExecuteJobs();

			// Merge all of the created GPUResourceEvent instances into one GPUResourceEventManager.
			for (auto&& [resourcePtr, resourceEventManager] : resourceEventManagerMap)
				mEventManager.MergeGPUResourceEventManager(std::move(resourceEventManager));
		}
	}
}