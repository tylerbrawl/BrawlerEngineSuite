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
import Brawler.D3D12.FrameGraphSyncPointFactory;

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

		Brawler::D3D12::FrameGraphSyncPointFactory syncPointFactory{};

		static constexpr auto ADD_RENDER_PASSES_LAMBDA = []<Brawler::D3D12::GPUCommandQueueType QueueType>(Brawler::D3D12::FrameGraphSyncPointFactory& syncPointFactory, const Brawler::D3D12::RenderPassBundle& bundle)
		{
			for (const auto& renderPassPtr : bundle.GetRenderPassSpan<QueueType>())
				syncPointFactory.AddResourceDependenciesForRenderPass(*renderPassPtr);
		};

		ADD_RENDER_PASSES_LAMBDA.operator()<Brawler::D3D12::GPUCommandQueueType::DIRECT>(syncPointFactory, bundle);
		ADD_RENDER_PASSES_LAMBDA.operator()<Brawler::D3D12::GPUCommandQueueType::COMPUTE>(syncPointFactory, bundle);
		ADD_RENDER_PASSES_LAMBDA.operator()<Brawler::D3D12::GPUCommandQueueType::COPY>(syncPointFactory, bundle);

		return syncPointFactory.CreateSyncPoint();
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

		FrameGraph& FrameGraphBuilder::GetFrameGraph()
		{
			assert(mFrameGraphPtr != nullptr);
			return *mFrameGraphPtr;
		}

		const FrameGraph& FrameGraphBuilder::GetFrameGraph() const
		{
			assert(mFrameGraphPtr != nullptr);
			return *mFrameGraphPtr;
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

		void FrameGraphBuilder::MergeFrameGraphBuilder(FrameGraphBuilder&& mergedBuilder)
		{
			assert(mFrameGraphPtr == mergedBuilder.mFrameGraphPtr);

			mRenderPassBundleArr.reserve(mRenderPassBundleArr.size() + mergedBuilder.mRenderPassBundleArr.size());

			for (auto&& bundle : mergedBuilder.mRenderPassBundleArr)
				mRenderPassBundleArr.push_back(std::move(bundle));

			mTransientResourceArr.reserve(mTransientResourceArr.size() + mergedBuilder.mTransientResourceArr.size());

			for (auto&& transientResource : mergedBuilder.mTransientResourceArr)
				mTransientResourceArr.push_back(std::move(transientResource));
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