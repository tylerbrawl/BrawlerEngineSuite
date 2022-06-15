module;
#include <vector>
#include <unordered_map>
#include <span>
#include <cassert>
#include <optional>
#include "DxDef.h"

export module Brawler.D3D12.FrameGraphSyncPointFactory;
import Brawler.D3D12.I_GPUResource;
import Util.General;
import Brawler.CompositeEnum;
import Brawler.D3D12.GPUCommandQueueType;
import Brawler.D3D12.I_RenderPass;
import Brawler.D3D12.FrameGraphResourceDependency;
import Brawler.D3D12.RenderPassBundle;

namespace Brawler
{
	namespace D3D12
	{
		template <Util::General::BuildMode BuildMode>
		struct SharedSubResourceInfo
		{
			D3D12_RESOURCE_STATES CombinedState;
			Brawler::CompositeEnum<GPUCommandQueueType> UsedQueues;

			template <GPUCommandQueueType QueueType>
			void UpdateInfoForResourceDependency(const FrameGraphResourceDependency& resourceDependency)
			{
				assert(!UsedQueues.ContainsAnyFlag(GPUCommandQueueType::COPY) && "ERROR: A sub-resource cannot be used simultaneously in both the DIRECT or COMPUTE queue and the COPY queue!");
				
				CombinedState |= resourceDependency.RequiredState;
				UsedQueues |= QueueType;
			}

			template <>
			void UpdateInfoForResourceDependency<GPUCommandQueueType::COPY>(const FrameGraphResourceDependency& resourceDependency)
			{
				// Resources must start out in the COMMON state when used in the COPY queue. We
				// allow API users to specify a valid state for the queue when adding a dependent
				// resource (such as D3D12_RESOURCE_STATE_COPY_SOURCE), but internally, we set the
				// actual state to use to be the COMMON state.

				assert(!UsedQueues.ContainsAnyFlag(GPUCommandQueueType::DIRECT | GPUCommandQueueType::COMPUTE) && "ERROR: A resource cannot be used simultaneously in both the DIRECT or COMPUTE queue and the COPY queue!");

				CombinedState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;
				UsedQueues = GPUCommandQueueType::COPY;
			}
		};

		template <>
		struct SharedSubResourceInfo<Util::General::BuildMode::DEBUG>
		{
			D3D12_RESOURCE_STATES CombinedState;
			Brawler::CompositeEnum<GPUCommandQueueType> UsedQueues;

			bool UsesExplicitCommonStateTransition;

			template <GPUCommandQueueType QueueType>
			void UpdateInfoForResourceDependency(const FrameGraphResourceDependency& resourceDependency)
			{
				CombinedState |= resourceDependency.RequiredState;
				UsedQueues |= QueueType;

				if (!UsesExplicitCommonStateTransition)
					UsesExplicitCommonStateTransition = (resourceDependency.RequiredState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON);
			}

			template <>
			void UpdateInfoForResourceDependency<GPUCommandQueueType::COPY>(const FrameGraphResourceDependency& resourceDependency)
			{
				// Resources must start out in the COMMON state when used in the COPY queue. We
				// allow API users to specify a valid state for the queue when adding a dependent
				// resource (such as D3D12_RESOURCE_STATE_COPY_SOURCE), but internally, we set the
				// actual state to use to be the COMMON state.

				assert(!UsedQueues.ContainsAnyFlag(GPUCommandQueueType::DIRECT | GPUCommandQueueType::COMPUTE) && "ERROR: A resource cannot be used simultaneously in both the DIRECT or COMPUTE queue and the COPY queue!");

				CombinedState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;
				UsedQueues = GPUCommandQueueType::COPY;
				UsesExplicitCommonStateTransition = true;
			}
		};

		using CurrentSharedSubResourceInfo = SharedSubResourceInfo<Util::General::GetBuildMode()>;

		class CrossQueueGPUResourceTracker
		{
		public:
			explicit CrossQueueGPUResourceTracker(I_GPUResource& resource);

			CrossQueueGPUResourceTracker(const CrossQueueGPUResourceTracker& rhs) = delete;
			CrossQueueGPUResourceTracker& operator=(const CrossQueueGPUResourceTracker& rhs) = delete;

			CrossQueueGPUResourceTracker(CrossQueueGPUResourceTracker&& rhs) noexcept = default;
			CrossQueueGPUResourceTracker& operator=(CrossQueueGPUResourceTracker&& rhs) noexcept = default;

			template <GPUCommandQueueType QueueType>
			void AddResourceDependency(const FrameGraphResourceDependency& resourceDependency);

			void AssertResourceUsageValidity() const;
			void AddSyncPointResourceDependencies(std::vector<FrameGraphResourceDependency>& resourceDependencyArr) const;

		private:
			std::vector<CurrentSharedSubResourceInfo> mSubResourceInfoArr;
			I_GPUResource* mResourcePtr;
		};
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class FrameGraphSyncPointFactory
		{
		public:
			FrameGraphSyncPointFactory() = default;

			FrameGraphSyncPointFactory(const FrameGraphSyncPointFactory& rhs) = delete;
			FrameGraphSyncPointFactory& operator=(const FrameGraphSyncPointFactory& rhs) = delete;

			FrameGraphSyncPointFactory(FrameGraphSyncPointFactory&& rhs) noexcept = default;
			FrameGraphSyncPointFactory& operator=(FrameGraphSyncPointFactory&& rhs) noexcept = default;

			template <GPUCommandQueueType QueueType>
			void AddResourceDependenciesForRenderPass(const I_RenderPass<QueueType>& renderPass);

			std::optional<RenderPassBundle> CreateSyncPoint() const;

		private:
			std::unordered_map<I_GPUResource*, CrossQueueGPUResourceTracker> mCrossQueueTrackerMap;
		};
	}
}

// ---------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType>
		void CrossQueueGPUResourceTracker::AddResourceDependency(const FrameGraphResourceDependency& resourceDependency)
		{
			assert(resourceDependency.SubResourceIndex < mSubResourceInfoArr.size() || resourceDependency.SubResourceIndex == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
			
			if (resourceDependency.SubResourceIndex == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
			{
				for (auto& subResourceInfo : mSubResourceInfoArr)
					subResourceInfo.UpdateInfoForResourceDependency<QueueType>(resourceDependency);
			}
			else
				mSubResourceInfoArr[resourceDependency.SubResourceIndex].UpdateInfoForResourceDependency<QueueType>(resourceDependency);
		}
	}
}

namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType>
		void FrameGraphSyncPointFactory::AddResourceDependenciesForRenderPass(const I_RenderPass<QueueType>& renderPass)
		{
			for (const auto& resourceDependency : renderPass.GetResourceDependencies())
			{
				if (!mCrossQueueTrackerMap.contains(resourceDependency.ResourcePtr))
					mCrossQueueTrackerMap.try_emplace(resourceDependency.ResourcePtr, *(resourceDependency.ResourcePtr));

				mCrossQueueTrackerMap.at(resourceDependency.ResourcePtr).AddResourceDependency<QueueType>(resourceDependency);
			}
		}
	}
}