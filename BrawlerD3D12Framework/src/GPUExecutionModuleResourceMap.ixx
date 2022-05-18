module;
#include <unordered_map>
#include <vector>
#include <span>
#include <algorithm>
#include <cassert>
#include "DxDef.h"

export module Brawler.D3D12.GPUExecutionModuleResourceMap;
import Brawler.D3D12.GPUCommandQueueType;
import Brawler.D3D12.I_RenderPass;
import Brawler.D3D12.FrameGraphResourceDependency;
import Brawler.D3D12.ResourceStateZone;

export namespace Brawler
{
	namespace D3D12
	{
		class I_GPUResource;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType>
		class GPUExecutionModuleResourceMap
		{
		private:
			struct RenderPassResourceUsageInfo
			{
				std::size_t RenderPassIndex;
				D3D12_RESOURCE_STATES RequiredState;
			};

		public:
			GPUExecutionModuleResourceMap() = default;

			GPUExecutionModuleResourceMap(const GPUExecutionModuleResourceMap& rhs) = delete;
			GPUExecutionModuleResourceMap& operator=(const GPUExecutionModuleResourceMap& rhs) = delete;

			GPUExecutionModuleResourceMap(GPUExecutionModuleResourceMap&& rhs) noexcept = default;
			GPUExecutionModuleResourceMap& operator=(GPUExecutionModuleResourceMap&& rhs) noexcept = default;

			void AddResourceDependenciesForRenderPass(const I_RenderPass<QueueType>& renderPass, const std::size_t renderPassIndex);
			void FinalizeResourceMap(const std::span<const std::unique_ptr<I_RenderPass<QueueType>>> renderPassPtrSpan);

			bool DoesResourceHaveDependentRenderPasses(const I_GPUResource& resource) const;
			std::vector<ResourceStateZone> GetResourceStateZones(const I_GPUResource& resource) const;

		private:
			std::unordered_map<const I_GPUResource*, std::vector<RenderPassResourceUsageInfo>> mRenderPassResourceUsageMap;
			std::span<const std::unique_ptr<I_RenderPass<QueueType>>> mRenderPassPtrSpan;
		};
	}
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType>
		void GPUExecutionModuleResourceMap<QueueType>::AddResourceDependenciesForRenderPass(const I_RenderPass<QueueType>& renderPass, const std::size_t renderPassIndex)
		{
			for (const auto& dependency : renderPass.GetResourceDependencies())
				mRenderPassResourceUsageMap[dependency.ResourcePtr].push_back(RenderPassResourceUsageInfo{
					.RenderPassIndex = renderPassIndex,
					.RequiredState = dependency.RequiredState
				});
		}

		template <GPUCommandQueueType QueueType>
		void GPUExecutionModuleResourceMap<QueueType>::FinalizeResourceMap(const std::span<const std::unique_ptr<I_RenderPass<QueueType>>> renderPassPtrSpan)
		{
			mRenderPassPtrSpan = std::move(renderPassPtrSpan);
		}

		template <GPUCommandQueueType QueueType>
		bool GPUExecutionModuleResourceMap<QueueType>::DoesResourceHaveDependentRenderPasses(const I_GPUResource& resource) const
		{
			return mRenderPassResourceUsageMap.contains(&resource);
		}

		template <GPUCommandQueueType QueueType>
		std::vector<ResourceStateZone> GPUExecutionModuleResourceMap<QueueType>::GetResourceStateZones(const I_GPUResource& resource) const
		{
			// We store the index of each I_RenderPass instance which uses an I_GPUResource instance
			// within mRenderPassIndexMap. Obviously, we need to create ResourceStateZone instances
			// for each of these render passes.
			//
			// However, in order to maximize split barrier potential, we should also always create a
			// ResourceStateZone for the render pass which immediately follows a render pass
			// described in mRenderPassIndexMap. If this render pass already happens to be in the map,
			// then we just have an immediate transition. If not, however, then we can create a null
			// ResourceStateZone.
			//
			// Lastly, in the case where a resource is not used in any I_RenderPass instance within
			// the corresponding GPUExecutionModule instance, we can return a single null
			// ResourceStateZone corresponding to the first I_RenderPass for this queue type, if
			// there are any render passes for this queue type in the first place.

			if (mRenderPassPtrSpan.empty()) [[unlikely]]
				return std::vector<ResourceStateZone>{};

			std::vector<ResourceStateZone> stateZoneArr{};

			// If no I_RenderPass instance within the corresponding GPUExecutionModule instance uses
			// resource, then return a single null ResourceStateZone for the very first I_RenderPass.
			// This maximizes the potential for split barriers.
			//
			// Profiling the Brawler Model Exporter for BC7 Image Compression shows that this path
			// is commonly taken, so we mark it as [[likely]] to benefit from branch prediction.
			if (!mRenderPassResourceUsageMap.contains(&resource)) [[likely]]
			{
				stateZoneArr.push_back(ResourceStateZone{
					.RequiredState{},
					.EntranceRenderPass{mRenderPassPtrSpan[0].get()},
					.QueueType = QueueType,
					.ExecutionModule{},
					.IsImplicitTransition = false,
					.IsDeleted = false
				});

				return stateZoneArr;
			}

			const std::span<const RenderPassResourceUsageInfo> resourceUsageInfoSpan{ mRenderPassResourceUsageMap.at(&resource) };

			// In the worst case, we will have one non-null ResourceStateZone for each index in 
			// resourceUsageInfoSpan and one null ResourceStateZone for each I_RenderPass
			// following a pass contained within said span. In other words, we will have at worst
			// (2 * resourceUsageInfoSpan.size()) ResourceStateZones to deal with.
			stateZoneArr.reserve(2 * resourceUsageInfoSpan.size());

			for (const auto& resourceUsageInfo : resourceUsageInfoSpan)
			{
				assert(resourceUsageInfo.RenderPassIndex < mRenderPassPtrSpan.size());

				const I_RenderPass<QueueType>& currRenderPass{ *(mRenderPassPtrSpan[resourceUsageInfo.RenderPassIndex]) };
				stateZoneArr.push_back(ResourceStateZone{
					.RequiredState{resourceUsageInfo.RequiredState},
					.EntranceRenderPass{&currRenderPass},
					.QueueType = QueueType,
					.ExecutionModule{},
					.IsImplicitTransition = false,
					.IsDeleted = false
				});

				const std::size_t nextRenderPassIndex = (resourceUsageInfo.RenderPassIndex + 1);

				// If the I_RenderPass immediately following the one specified by resourceUsageInfo
				// is not using resource, then add a null ResourceStateZone for it. Since we know
				// that resourceUsageInfoSpan must inherently be sorted in order of increasing index
				// based on how resource dependencies are added, we can use std::ranges::lower_bound
				// to dramatically improve search performance.
				if (nextRenderPassIndex < mRenderPassPtrSpan.size() &&
					std::ranges::lower_bound(resourceUsageInfoSpan, nextRenderPassIndex, std::ranges::less{}, [](const RenderPassResourceUsageInfo& usageInfo) { return usageInfo.RenderPassIndex; }) == resourceUsageInfoSpan.end())
					stateZoneArr.push_back(ResourceStateZone{
						.RequiredState{},
						.EntranceRenderPass{mRenderPassPtrSpan[nextRenderPassIndex].get()},
						.QueueType = QueueType,
						.ExecutionModule{},
						.IsImplicitTransition = false,
						.IsDeleted = false
					});
			}

			return stateZoneArr;
		}
	}
}