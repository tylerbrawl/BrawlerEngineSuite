module;
#include <optional>
#include <span>
#include "DxDef.h"

export module Brawler.D3D12.ImplicitBarrierGPUResourceStateTrackerState;
import Brawler.D3D12.I_GPUResourceStateTrackerState;
import Brawler.OptionalRef;
import Brawler.D3D12.I_RenderPass;
import Brawler.D3D12.GPUResourceStateTrackingContext;
import Brawler.D3D12.FrameGraphResourceDependency;
import Util.D3D12;
import Brawler.D3D12.GPUResourceStateBarrierMerger;

export namespace Brawler
{
	namespace D3D12
	{
		class ImplicitBarrierGPUResourceStateTrackerState final : public I_GPUResourceStateTrackerState<ImplicitBarrierGPUResourceStateTrackerState>
		{
		public:
			ImplicitBarrierGPUResourceStateTrackerState() = default;

			ImplicitBarrierGPUResourceStateTrackerState(const ImplicitBarrierGPUResourceStateTrackerState& rhs) = delete;
			ImplicitBarrierGPUResourceStateTrackerState& operator=(const ImplicitBarrierGPUResourceStateTrackerState& rhs) = delete;

			ImplicitBarrierGPUResourceStateTrackerState(ImplicitBarrierGPUResourceStateTrackerState&& rhs) noexcept = default;
			ImplicitBarrierGPUResourceStateTrackerState& operator=(ImplicitBarrierGPUResourceStateTrackerState&& rhs) noexcept = default;

			template <GPUCommandQueueType QueueType>
			void TrackRenderPass(const I_RenderPass<QueueType>& renderPass, const GPUResourceStateTrackingContext& context);

			bool WillResourceStateDecay(const GPUResourceStateTrackingContext& context) const;
			std::optional<GPUResourceStateTrackerStateID> GetNextStateID() const;

		private:
			bool mTransitionToExplicitBarrierState;
			bool mImplicitTransitionsOnlyToReadStates;
		};
	}
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType>
		void ImplicitBarrierGPUResourceStateTrackerState::TrackRenderPass(const I_RenderPass<QueueType>& renderPass, const GPUResourceStateTrackingContext& context)
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

			// Try to "eat" the transition barrier, if we can.
			if (currResourceDependency->RequiredState != D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON || context.GPUResource.GetCurrentResourceState() == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON)
			{
				const D3D12_RESOURCE_STATES proposedImplicitState = (context.GPUResource.GetCurrentResourceState() | currResourceDependency->RequiredState);

				if (Util::D3D12::IsImplicitStateTransitionPossible(context.GPUResource, proposedImplicitState))
				{
					context.GPUResource.SetCurrentResourceState(proposedImplicitState);
					mImplicitTransitionsOnlyToReadStates = Util::D3D12::IsValidReadState(proposedImplicitState);

					return;
				}
			}

			// If we can't do that, then we need to add an explicit state transition.
			context.BarrierMerger.AddExplicitStateTransition(renderPass, currResourceDependency->RequiredState);
			mTransitionToExplicitBarrierState = true;
		}

		bool ImplicitBarrierGPUResourceStateTrackerState::WillResourceStateDecay(const GPUResourceStateTrackingContext& context) const
		{
			// If the resource was implicitly promoted to a read-only state, then it can also decay back
			// to the COMMON state.
			return (CheckGeneralResourceStateDecayRequirements(context) || mImplicitTransitionsOnlyToReadStates);
		}

		std::optional<GPUResourceStateTrackerStateID> ImplicitBarrierGPUResourceStateTrackerState::GetNextStateID() const
		{
			return (mTransitionToExplicitBarrierState ? std::optional<GPUResourceStateTrackerStateID>{ GPUResourceStateTrackerStateID::EXPLICIT_BARRIER } : std::optional<GPUResourceStateTrackerStateID>{});
		}
	}
}