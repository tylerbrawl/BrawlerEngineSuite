module;
#include <vector>
#include <memory>
#include <cassert>
#include <optional>
#include <unordered_map>
#include "DxDef.h"

module Brawler.D3D12.FrameGraphBuilder;
import Brawler.D3D12.FrameGraph;
import Brawler.D3D12.TransientGPUResourceAliasTracker;
import Brawler.CompositeEnum;
import Util.D3D12;

namespace
{
	std::optional<Brawler::D3D12::RenderPassBundle> CreateSyncPointForRenderPassBundle(const Brawler::D3D12::RenderPassBundle& bundle)
	{
		// When we create a sync point for a RenderPassBundle, we are creating a RenderPassBundle
		// with one empty direct RenderPass. This "RenderPass" will contain a resource dependency
		// for each resource shared between multiple queues, and this dependency will be the
		// combination of all required states across all of the RenderPass instances in the
		// RenderPassBundle.
		// 
		// The idea behind doing it like this is that a resource should never be used in multiple
		// queues within the same RenderPassBundle instance, unless the resource is only ever
		// used with read-only access. In that case, we can merge all of the required read states
		// and enforce the resource to be in the correct state here.
		//
		// In addition, the D3D12 API requires that resources which are to be used in the copy
		// queue must begin in the COMMON state. We can use a sync point to thus also transition
		// resources to this state before they are used in the copy queue.
		//
		// It is unclear in the MSDN as to whether or not a resource can be used simultaneously in
		// both the copy queue and in the direct and/or compute queue(s). However, it seems to imply
		// that this is *NOT* valid (see 
		// https://docs.microsoft.com/en-us/windows/win32/direct3d12/user-mode-heap-synchronization#multi-queue-resource-access).

		// Do not create a sync point for RenderPassBundle instances which contain only direct
		// RenderPass instances. This is the common case and the so-called "fast path" (implemented
		// via the [[likely]] attribute).
		if ((bundle.GetRenderPassCount<Brawler::D3D12::GPUCommandQueueType::COMPUTE>() + bundle.GetRenderPassCount<Brawler::D3D12::GPUCommandQueueType::COPY>()) == 0) [[likely]]
			return std::optional<Brawler::D3D12::RenderPassBundle>{};

		struct SharedResourceInfo
		{
			D3D12_RESOURCE_STATES CombinedState;
			Brawler::CompositeEnum<Brawler::D3D12::GPUCommandQueueType> UsedQueues;

			/*
			There is a bit of a problem with allowing transitions to the COMMON state for
			resources being used in multiple queues. Obviously, since the COMMON state allows a
			resource to implicitly be transitioned to a write state, we can't allow it for resources
			being used simultaneously across multiple queues.

			However, there is no easy way to check for this, because the underlying value of the
			COMMON state is 0. Let VALID_READ_STATE_SET be a valid read-only resource state.
			Then, Util::D3D12::IsValidReadState(VALID_READ_STATE_SET | D3D12_RESOURCE_STATE_COMMON)
			would return true. This still works out okay, actually, because
			VALID_READ_STATE_SET | D3D12_RESOURCE_STATE_COMMON == VALID_READ_STATE_SET.

			The problem is that since we assume that all of the states which a resource will be
			in if it is being used simultaneously across queues are read-only states, and since
			we can combine an arbitrary number of read-only states, we group all of these transitions
			into one read-only state specified in the sync point preceding the current
			GPUExecutionModule.

			If the API user sets the resource dependency in one RenderPass as the COMMON state and
			uses read states everywhere else, then after we combine all of the read states, the
			COMMON state dependency is effectively lost. This is technically fine, since the API
			user invoked undefined behavior by specifying the COMMON state in the first place.

			However, we will still assert in Debug builds that they aren't doing this. (I'm a big
			believer in Debug builds having an excess of error checking, anyways.)
			*/
#ifdef _DEBUG
			bool UsesExplicitCommonStateTransition;
#endif // _DEBUG
		};

		std::unordered_map<Brawler::D3D12::I_GPUResource*, SharedResourceInfo> resourceStateMap{};

		const auto addDependenciesLambda = []<Brawler::D3D12::GPUCommandQueueType QueueType>(std::unordered_map<Brawler::D3D12::I_GPUResource*, SharedResourceInfo>& stateMap, const Brawler::D3D12::RenderPassBundle& bundle)
		{
			for (const auto& renderPass : bundle.GetRenderPassSpan<QueueType>())
			{
				for (const auto& dependency : renderPass->GetResourceDependencies())
				{
					stateMap[dependency.ResourcePtr].CombinedState |= dependency.RequiredState;
					stateMap[dependency.ResourcePtr].UsedQueues |= QueueType;

#ifdef _DEBUG
					if (!stateMap[dependency.ResourcePtr].UsesExplicitCommonStateTransition)
						stateMap[dependency.ResourcePtr].UsesExplicitCommonStateTransition = (dependency.RequiredState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON);
#endif // _DEBUG
				}
			}
		};

		addDependenciesLambda.operator()<Brawler::D3D12::GPUCommandQueueType::DIRECT>(resourceStateMap, bundle);
		addDependenciesLambda.operator()<Brawler::D3D12::GPUCommandQueueType::COMPUTE>(resourceStateMap, bundle);

		// Erase all SharedResourceInfo instances from resourceStateMap which do not correspond to
		// resources being used in both a direct and compute queue.
		std::erase_if(resourceStateMap, [] (const auto& keyValuePair)
		{
			static constexpr Brawler::CompositeEnum<Brawler::D3D12::GPUCommandQueueType> IS_SHARED_ACROSS_VALID_QUEUES = Brawler::D3D12::GPUCommandQueueType::DIRECT | Brawler::D3D12::GPUCommandQueueType::COMPUTE;

			const auto& [resourcePtr, resourceInfo] = keyValuePair;
			return (resourceInfo.UsedQueues != IS_SHARED_ACROSS_VALID_QUEUES);
		});

		// We should make sure that combining the states results in a valid final resource state.
		// If this is not the case, then the API user is illegally writing to a resource which
		// is shared between queues.
#ifdef _DEBUG
		for (const auto& [resourcePtr, resourceInfo] : resourceStateMap)
		{
			assert(Util::D3D12::IsValidReadState(resourceInfo.CombinedState) && "ERROR: A resource was marked as a dependency across multiple queues within a single RenderPassBundle, but at least one of them attempted to request write access to the resource!");
			assert(!resourceInfo.UsesExplicitCommonStateTransition && "ERROR: It is illegal to specify the D3D12_RESOURCE_STATE_COMMON state (or the D3D12_RESOURCE_STATE_PRESENT state) for a resource if the resource is being used simultaneously across multiple queues! (Doing so will invoke undefined behavior.)");
		}
#endif // _DEBUG

		// Now, add any resources which are used in the copy queue. We need these resources to be
		// in the D3D12_RESOURCE_STATE_COMMON state before we can make use of them there.
		for (const auto& copyPass : bundle.GetRenderPassSpan<Brawler::D3D12::GPUCommandQueueType::COPY>())
		{
			for (const auto& dependency : copyPass->GetResourceDependencies())
			{
				resourceStateMap[dependency.ResourcePtr].CombinedState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;
				resourceStateMap[dependency.ResourcePtr].UsedQueues = Brawler::D3D12::GPUCommandQueueType::COPY;
			}
		}

		// If we do not have any required resource transitions, then we do not necessarily
		// need a sync point.
		if (resourceStateMap.empty())
			return std::optional<Brawler::D3D12::RenderPassBundle>{};

		// Create the sync point.
		Brawler::D3D12::RenderPass<Brawler::D3D12::GPUCommandQueueType::DIRECT> syncPointPass{};

		for (const auto& [resourcePtr, resourceInfo] : resourceStateMap)
			syncPointPass.AddResourceDependency(*resourcePtr, resourceInfo.CombinedState);

		Brawler::D3D12::RenderPassBundle syncPointBundle{};
		syncPointBundle.AddDirectRenderPass(std::move(syncPointPass));

		return std::optional<Brawler::D3D12::RenderPassBundle>{ std::move(syncPointBundle) };
	}
}

namespace Brawler
{
	namespace D3D12
	{
		FrameGraphBuilder::FrameGraphBuilder(FrameGraph& frameGraph) :
			mRenderPassBundleArr(),
			mTransientResourceArr(),
			mFrameGraphPtr(&frameGraph)
		{}
		
		void FrameGraphBuilder::AddRenderPassBundle(RenderPassBundle&& bundle)
		{
			mRenderPassBundleArr.push_back(std::move(bundle));
		}

		std::size_t FrameGraphBuilder::GetRenderPassBundleCount() const
		{
			return mRenderPassBundleArr.size();
		}

		FrameGraphBlackboard& FrameGraphBuilder::GetBlackboard()
		{
			assert(mFrameGraphPtr != nullptr);
			return mFrameGraphPtr->GetBlackboard();
		}

		const FrameGraphBlackboard& FrameGraphBuilder::GetBlackboard() const
		{
			assert(mFrameGraphPtr != nullptr);
			return mFrameGraphPtr->GetBlackboard();
		}

		std::span<RenderPassBundle> FrameGraphBuilder::GetRenderPassBundleSpan()
		{
			return std::span<RenderPassBundle>{ mRenderPassBundleArr };
		}

		std::vector<std::unique_ptr<I_GPUResource>> FrameGraphBuilder::ExtractTransientResources()
		{
			return std::move(mTransientResourceArr);
		}

		void FrameGraphBuilder::SetRenderPassBundleIDs(std::uint32_t baseID)
		{
			for (auto& bundle : mRenderPassBundleArr)
				bundle.SetRenderPassBundleID(baseID++);
		}

		void FrameGraphBuilder::UpdateTransientGPUResourceAliasTracker(TransientGPUResourceAliasTracker& aliasTracker) const
		{
			for (const auto& bundle : mRenderPassBundleArr)
				bundle.UpdateTransientGPUResourceAliasTracker(aliasTracker);
		}

		void FrameGraphBuilder::CreateSyncPoints()
		{
			for (auto itr = mRenderPassBundleArr.begin(); itr != mRenderPassBundleArr.end(); ++itr)
			{
				std::optional<RenderPassBundle> syncPointBundle{ CreateSyncPointForRenderPassBundle(*itr) };

				if (syncPointBundle.has_value())
				{
					syncPointBundle->MarkAsSyncPoint();

					itr = mRenderPassBundleArr.insert(itr, std::move(*syncPointBundle));
					++itr;
				}
			}
		}

		void FrameGraphBuilder::UpdateResourceUsageForCurrentFrame()
		{
			for (const auto& bundle : mRenderPassBundleArr)
			{
				for (const auto resourcePtr : bundle.GetResourceDependencies())
				{
					resourcePtr->MarkAsUsedForCurrentFrame();
					mResourceDependencyCache.Insert(resourcePtr);
				}					
			}
		}

		Brawler::SortedVector<I_GPUResource*> FrameGraphBuilder::ExtractResourceDependencyCache()
		{
			return std::move(mResourceDependencyCache);
		}
	}
}