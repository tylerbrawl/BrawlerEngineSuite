module;
#include <variant>
#include <optional>
#include <span>
#include <array>
#include "DxDef.h"

export module Brawler.D3D12.GPUSubResourceStateBarrierMerger;
import :BarrierMergerStateContainer;
import Brawler.D3D12.GPUResourceEventCollection;
import Brawler.D3D12.GPUResourceEvent;
import Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.GPUCommandQueueType;
import Brawler.D3D12.FrameGraphResourceDependency;
import Brawler.D3D12.I_RenderPass;
import Brawler.OptionalRef;

export namespace Brawler
{
	namespace D3D12
	{
		class GPUSubResourceStateBarrierMerger
		{
		private:
			using RenderPassVariant = std::variant<const I_RenderPass<GPUCommandQueueType::DIRECT>*, const I_RenderPass<GPUCommandQueueType::COMPUTE>*, const I_RenderPass<GPUCommandQueueType::COPY>*>;

		public:
			GPUSubResourceStateBarrierMerger(I_GPUResource& resource, const std::uint32_t subResourceIndex);

			GPUSubResourceStateBarrierMerger(const GPUSubResourceStateBarrierMerger& rhs) = delete;
			GPUSubResourceStateBarrierMerger& operator=(const GPUSubResourceStateBarrierMerger& rhs) = delete;

			GPUSubResourceStateBarrierMerger(GPUSubResourceStateBarrierMerger&& rhs) noexcept = default;
			GPUSubResourceStateBarrierMerger& operator=(GPUSubResourceStateBarrierMerger&& rhs) noexcept = default;

			I_GPUResource& GetGPUResource();
			std::uint32_t GetSubResourceIndex() const;

			template <GPUCommandQueueType QueueType>
			void TrackRenderPass(const I_RenderPass<QueueType>& renderPass);

			bool DoImplicitReadStateTransitionsAllowStateDecay() const;
			void DecayResourceState();

			GPUResourceEventCollection FinalizeStateTracking();

			template <GPUCommandQueueType QueueType>
			void AddPotentialBeginBarrierRenderPass(const I_RenderPass<QueueType>& renderPass);

			bool CanUseAdditionalBeginBarrierRenderPasses() const;

		private:
			void AddPotentialBeginBarrierRenderPass(RenderPassVariant passVariant);

			template <GPUCommandQueueType QueueType>
			void HandleResourceDependency(const I_RenderPass<QueueType>& renderPass, const D3D12_RESOURCE_STATES requiredState);

			template <GPUCommandQueueType QueueType>
			void CheckForOtherGPUResourceEvents(const I_RenderPass<QueueType>& renderPass, const D3D12_RESOURCE_STATES requiredState);

			void CommitExistingExplicitResourceTransition();
			void EraseBarrierPasses();

		private:
			BarrierMergerStateContainer mStateContainer;
			GPUResourceEventCollection mEventCollection;
			std::array<std::optional<RenderPassVariant>, 3> mBeginBarrierPassArr;
			std::optional<RenderPassVariant> mEndBarrierPass;
			bool mWasLastStateForUAV;
		};
	}
}

// ------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType>
		void GPUSubResourceStateBarrierMerger::TrackRenderPass(const I_RenderPass<QueueType>& renderPass)
		{
			const std::span<const FrameGraphResourceDependency> dependencySpan{ renderPass.GetResourceDependencies() };
			I_GPUResource* const resourcePtr = std::addressof(mStateContainer.GetGPUResource());

			const Brawler::OptionalRef<const FrameGraphResourceDependency> currDependency{ [this, resourcePtr, &dependencySpan]()
			{
				for (const auto& dependency : dependencySpan)
				{
					if (dependency.ResourcePtr == resourcePtr && dependency.SubResourceIndex == GetSubResourceIndex())
						return Brawler::OptionalRef<const FrameGraphResourceDependency>{ dependency };
				}

				return Brawler::OptionalRef<const FrameGraphResourceDependency>{};
			}() };

			if (currDependency.HasValue())
				HandleResourceDependency(renderPass, currDependency->RequiredState);
			else
				AddPotentialBeginBarrierRenderPass(RenderPassVariant{ std::addressof(renderPass) });
		}

		template <GPUCommandQueueType QueueType>
		void GPUSubResourceStateBarrierMerger::AddPotentialBeginBarrierRenderPass(const I_RenderPass<QueueType>& renderPass)
		{
			AddPotentialBeginBarrierRenderPass(RenderPassVariant{ std::addressof(renderPass) });
		}

		template <GPUCommandQueueType QueueType>
		void GPUSubResourceStateBarrierMerger::HandleResourceDependency(const I_RenderPass<QueueType>& renderPass, const D3D12_RESOURCE_STATES requiredState)
		{
			CheckForOtherGPUResourceEvents(renderPass, requiredState);

			// First, try to merge the existing resource state.
			// 
			// If we cannot do that, then commit the existing transition and
			// set the new after state.
			if (!mStateContainer.CanAfterStateBeCombined(requiredState))
				CommitExistingExplicitResourceTransition();

			// If we perform an implicit state promotion, we need to delete any previous potential
			// begin barrier passes.
			if (mStateContainer.IsImplicitTransitionPossible(requiredState))
				EraseBarrierPasses();

			mStateContainer.UpdateAfterState(requiredState);

			if (mStateContainer.HasExplicitStateTransition() && !mEndBarrierPass.has_value())
				mEndBarrierPass = RenderPassVariant{ std::addressof(renderPass) };
		}

		template <GPUCommandQueueType QueueType>
		void GPUSubResourceStateBarrierMerger::CheckForOtherGPUResourceEvents(const I_RenderPass<QueueType>& renderPass, const D3D12_RESOURCE_STATES requiredState)
		{
			I_GPUResource& resource{ mStateContainer.GetGPUResource() };

			// Only the DIRECT queue is capable of performing special GPU resource initialization.
			if (resource.RequiresSpecialInitialization()) [[unlikely]]
			{
				mEventCollection.AddGPUResourceEvent(renderPass, GPUResourceEvent{
					.GPUResource{std::addressof(resource)},
					.Event{ResourceInitializationEvent{}},
					.EventID = GPUResourceEventID::RESOURCE_INITIALIZATION
				});

				resource.MarkSpecialInitializationAsCompleted();
			}
			
			const bool thisStateIsForUAV = (requiredState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			if (thisStateIsForUAV && mWasLastStateForUAV) [[unlikely]]
			{
				mEventCollection.AddGPUResourceEvent(renderPass, GPUResourceEvent{
					.GPUResource{std::addressof(resource)},
					.Event{UAVBarrierEvent{}},
					.EventID = GPUResourceEventID::UAV_BARRIER
				});
			}

			mWasLastStateForUAV = thisStateIsForUAV;
		}
	}
}