module;
#include <optional>
#include "DxDef.h"

export module Brawler.D3D12.I_GPUResourceStateTrackerState;
import Brawler.D3D12.GPUCommandQueueType;
import Brawler.D3D12.GPUResourceStateTrackerStateID;
import Brawler.D3D12.GPUResourceStateTrackingContext;
import Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.GPUExecutionModule;

export namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType>
		class I_RenderPass;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		template <typename DerivedType>
		class I_GPUResourceStateTrackerState
		{
		protected:
			I_GPUResourceStateTrackerState() = default;

		public:
			virtual ~I_GPUResourceStateTrackerState() = default;

			I_GPUResourceStateTrackerState(const I_GPUResourceStateTrackerState& rhs) = delete;
			I_GPUResourceStateTrackerState& operator=(const I_GPUResourceStateTrackerState& rhs) = delete;

			I_GPUResourceStateTrackerState(I_GPUResourceStateTrackerState&& rhs) noexcept = default;
			I_GPUResourceStateTrackerState& operator=(I_GPUResourceStateTrackerState&& rhs) noexcept = default;

			template <GPUCommandQueueType QueueType>
			void TrackRenderPass(const I_RenderPass<QueueType>& renderPass, const GPUResourceStateTrackingContext& context);

			bool WillResourceStateDecay(const GPUResourceStateTrackingContext& context) const;
			std::optional<GPUResourceStateTrackerStateID> GetNextStateID() const;

		protected:
			bool CheckGeneralResourceStateDecayRequirements(const GPUResourceStateTrackingContext& context) const;
		};
	}
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <typename DerivedType>
		template <GPUCommandQueueType QueueType>
		void I_GPUResourceStateTrackerState<DerivedType>::TrackRenderPass(const I_RenderPass<QueueType>& renderPass, const GPUResourceStateTrackingContext& context)
		{
			static_cast<DerivedType*>(this)->TrackRenderPass(renderPass, context);
		}

		template <typename DerivedType>
		bool I_GPUResourceStateTrackerState<DerivedType>::WillResourceStateDecay(const GPUResourceStateTrackingContext& context) const
		{
			return static_cast<DerivedType*>(this)->WillResourceStateDecay(context);
		}

		template <typename DerivedType>
		std::optional<GPUResourceStateTrackerStateID> I_GPUResourceStateTrackerState<DerivedType>::GetNextStateID() const
		{
			return static_cast<DerivedType*>(this)->GetNextStateID();
		}

		template <typename DerivedType>
		bool I_GPUResourceStateTrackerState<DerivedType>::CheckGeneralResourceStateDecayRequirements(const GPUResourceStateTrackingContext& context) const
		{
			// Buffers and simultaneous-access textures will always decay.
			{
				const Brawler::D3D12_RESOURCE_DESC& resourceDesc{ context.GPUResource.GetResourceDescription() };

				if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER || (resourceDesc.Flags & D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS) != 0)
					return true;
			}

			// Resources accessed in a COPY queue will also decay to the COMMON state.
			return context.ExecutionModule.IsResourceUsedInQueue<GPUCommandQueueType::COPY>(context.GPUResource);
		}
	}
}