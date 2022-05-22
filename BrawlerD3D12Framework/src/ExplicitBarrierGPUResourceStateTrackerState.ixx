module;
#include <span>
#include <optional>
#include "DxDef.h"

export module Brawler.D3D12.ExplicitBarrierGPUResourceStateTrackerState;
import Brawler.D3D12.I_GPUResourceStateTrackerState;
import Brawler.D3D12.GPUCommandQueueType;
import Brawler.D3D12.I_RenderPass;
import Brawler.D3D12.GPUResourceStateTrackingContext;
import Brawler.D3D12.GPUResourceStateBarrierMerger;
import Brawler.OptionalRef;
import Brawler.D3D12.FrameGraphResourceDependency;

export namespace Brawler
{
	namespace D3D12
	{
		class ExplicitBarrierGPUResourceStateTrackerState final : public I_GPUResourceStateTrackerState<ExplicitBarrierGPUResourceStateTrackerState>
		{
		public:
			ExplicitBarrierGPUResourceStateTrackerState() = default;

			ExplicitBarrierGPUResourceStateTrackerState(const ExplicitBarrierGPUResourceStateTrackerState& rhs) = delete;
			ExplicitBarrierGPUResourceStateTrackerState& operator=(const ExplicitBarrierGPUResourceStateTrackerState& rhs) = delete;

			ExplicitBarrierGPUResourceStateTrackerState(ExplicitBarrierGPUResourceStateTrackerState&& rhs) noexcept = default;
			ExplicitBarrierGPUResourceStateTrackerState& operator=(ExplicitBarrierGPUResourceStateTrackerState&& rhs) noexcept = default;

			template <GPUCommandQueueType QueueType>
			void TrackRenderPass(const I_RenderPass<QueueType>& renderPass, const GPUResourceStateTrackingContext& context);

			bool WillResourceStateDecay(const GPUResourceStateTrackingContext& context) const;
			std::optional<GPUResourceStateTrackerStateID> GetNextStateID() const;

		private:
			bool mTransitionToImplicitBarrierState;
		};
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType>
		void ExplicitBarrierGPUResourceStateTrackerState::TrackRenderPass(const I_RenderPass<QueueType>& renderPass, const GPUResourceStateTrackingContext& context)
		{
			const Brawler::OptionalRef<const FrameGraphResourceDependency> currResourceDependency{ [](const std::span<const FrameGraphResourceDependency> dependencySpan, const I_GPUResource& resource)
			{
				for (const auto& dependency : dependencySpan)
				{
					if (dependency.ResourcePtr == std::addressof(resource))
						return Brawler::OptionalRef<const FrameGraphResourceDependency>{dependency};
				}

				return Brawler::OptionalRef<const FrameGraphResourceDependency>{};
			}(renderPass.GetResourceDependencies(), context.GPUResource) };

			if (!currResourceDependency.HasValue())
			{
				context.BarrierMerger.AddPotentialSplitBarrierBeginRenderPass(renderPass);
				return;
			}

			context.BarrierMerger.AddExplicitStateTransition(renderPass, currResourceDependency->RequiredState);

			if (currResourceDependency->RequiredState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON)
				mTransitionToImplicitBarrierState = true;
		}

		bool ExplicitBarrierGPUResourceStateTrackerState::WillResourceStateDecay(const GPUResourceStateTrackingContext& context) const
		{
			return CheckGeneralResourceStateDecayRequirements(context);
		}

		std::optional<GPUResourceStateTrackerStateID> ExplicitBarrierGPUResourceStateTrackerState::GetNextStateID() const
		{
			return (mTransitionToImplicitBarrierState ? std::optional<GPUResourceStateTrackerStateID>{ GPUResourceStateTrackerStateID::IMPLICIT_BARRIER } : std::optional<GPUResourceStateTrackerStateID>{});
		}
	}
}