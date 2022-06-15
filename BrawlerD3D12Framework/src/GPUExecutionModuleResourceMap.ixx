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
import Brawler.D3D12.I_GPUResource;

export namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType>
		class GPUExecutionModuleResourceMap
		{
		private:
			using SubResourceUsageInfoArray = std::vector<bool>;

		public:
			GPUExecutionModuleResourceMap() = default;

			GPUExecutionModuleResourceMap(const GPUExecutionModuleResourceMap& rhs) = delete;
			GPUExecutionModuleResourceMap& operator=(const GPUExecutionModuleResourceMap& rhs) = delete;

			GPUExecutionModuleResourceMap(GPUExecutionModuleResourceMap&& rhs) noexcept = default;
			GPUExecutionModuleResourceMap& operator=(GPUExecutionModuleResourceMap&& rhs) noexcept = default;

			void AddResourceDependenciesForRenderPass(const I_RenderPass<QueueType>& renderPass);

			bool DoesResourceHaveDependentRenderPasses(const I_GPUResource& resource) const;
			bool DoesResourceHaveDependentRenderPasses(const I_GPUResource& resource, const std::uint32_t subResourceIndex) const;

		private:
			std::unordered_map<const I_GPUResource*, std::vector<bool>> mRenderPassResourceUsageMap;
		};
	}
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType>
		void GPUExecutionModuleResourceMap<QueueType>::AddResourceDependenciesForRenderPass(const I_RenderPass<QueueType>& renderPass)
		{
			for (const auto& dependency : renderPass.GetResourceDependencies())
			{
				const I_GPUResource* const resourcePtr = dependency.ResourcePtr;
				
				if (!mRenderPassResourceUsageMap.contains(resourcePtr))
					mRenderPassResourceUsageMap[resourcePtr].resize(resourcePtr->GetSubResourceCount());

				mRenderPassResourceUsageMap[resourcePtr][dependency.SubResourceIndex] = true;
			}
		}

		template <GPUCommandQueueType QueueType>
		bool GPUExecutionModuleResourceMap<QueueType>::DoesResourceHaveDependentRenderPasses(const I_GPUResource& resource) const
		{
			return mRenderPassResourceUsageMap.contains(&resource);
		}

		template <GPUCommandQueueType QueueType>
		bool GPUExecutionModuleResourceMap<QueueType>::DoesResourceHaveDependentRenderPasses(const I_GPUResource& resource, const std::uint32_t subResourceIndex) const
		{
			// Most render passes likely won't contain every I_GPUResource.
			if (!mRenderPassResourceUsageMap.contains(&resource)) [[likely]]
				return false;

			assert(subResourceIndex < mRenderPassResourceUsageMap.at(&resource).size());
			return mRenderPassResourceUsageMap.at(&resource)[subResourceIndex];
		}
	}
}