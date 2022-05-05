module;
#include <variant>
#include <optional>
#include "DxDef.h"

export module Brawler.D3D12.ResourceStateZone;
import Brawler.D3D12.GPUCommandQueueType;

export namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType>
		class I_RenderPass;

		class GPUExecutionModule;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		struct ResourceStateZone
		{
			std::optional<D3D12_RESOURCE_STATES> RequiredState;

			std::variant<const I_RenderPass<GPUCommandQueueType::DIRECT>*, const I_RenderPass<GPUCommandQueueType::COMPUTE>*, const I_RenderPass<GPUCommandQueueType::COPY>*> EntranceRenderPass;
			GPUCommandQueueType QueueType;

			const GPUExecutionModule* ExecutionModule;

			bool IsImplicitTransition;

			/// <summary>
			/// Checks whether this ResourceStateZone instance is a null ResourceStateZone. A null
			/// ResourceStateZone does not have a required resource state for a given resource.
			/// 
			/// Upon entering a null ResourceStateZone, one can place a begin split barrier for
			/// the resource state needed for the next non-null ResourceStateZone.
			/// </summary>
			/// <returns>
			/// The function returns true if this ResourceStateZone instance is a null ResourceStateZone
			/// and false otherwise.
			/// </returns>
			__forceinline bool IsNull() const
			{
				return !RequiredState.has_value();
			}
		};
	}
}