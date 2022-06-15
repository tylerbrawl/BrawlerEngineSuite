module;
#include <optional>
#include <cassert>
#include "DxDef.h"

export module Brawler.D3D12.GPUSubResourceStateBarrierMerger:BarrierMergerStateContainer;
import Brawler.D3D12.I_GPUResource;
import Util.D3D12;

export namespace Brawler
{
	namespace D3D12
	{
		class BarrierMergerStateContainer
		{
		public:
			BarrierMergerStateContainer(I_GPUResource& resource, const std::uint32_t subResourceIndex);

			BarrierMergerStateContainer(const BarrierMergerStateContainer& rhs) = delete;
			BarrierMergerStateContainer& operator=(const BarrierMergerStateContainer& rhs) = delete;

			BarrierMergerStateContainer(BarrierMergerStateContainer&& rhs) noexcept = default;
			BarrierMergerStateContainer& operator=(BarrierMergerStateContainer&& rhs) noexcept = default;

			I_GPUResource& GetGPUResource();
			std::uint32_t GetSubResourceIndex() const;

			/// <summary>
			/// Determines whether or not requiredState can be combined with the value currently
			/// stored as the after state for this BarrierMergerStateContainer. If no value is
			/// currently stored, then this function returns true.
			/// </summary>
			/// <param name="requiredState">
			/// - The state which is to be combined with the current after state.
			/// </param>
			/// <returns>
			/// The function returns true if requiredState can be combined with the value currently
			/// stored as the after state for this BarrierMergerStateContainer and false otherwise.
			/// 
			/// If no value is currently stored, then the function returns true.
			/// </returns>
			bool CanAfterStateBeCombined(const D3D12_RESOURCE_STATES requiredState) const;

			void UpdateAfterState(const D3D12_RESOURCE_STATES requiredState);

			/// <summary>
			/// Determines whether or not the resource state will decay as a result of the most complex
			/// resource state decay rule in the D3D12 API, which reads as follows:
			/// 
			/// The state of a resource will decay after a call to ID3D12CommandQueue::ExecuteCommandLists()
			/// if it was implicitly promoted to a read-only state.
			/// </summary>
			/// <returns>
			/// The function returns true if the resource was only ever implicitly promoted to a read-only
			/// state and false otherwise. If this function returns true, then it is guaranteed that the
			/// associated resource will have its state decay back to the COMMON state.
			/// </returns>
			bool DoImplicitReadStateTransitionsAllowStateDecay() const;

			bool IsImplicitTransitionPossible(const D3D12_RESOURCE_STATES requiredState) const;

			bool HasExplicitStateTransition() const;

			D3D12_RESOURCE_STATES GetBeforeState() const;
			D3D12_RESOURCE_STATES GetAfterState() const;

			void UpdateGPUResourceState();

			void SwapStates();
			void DecayResourceState();

		private:
			I_GPUResource* mResourcePtr;
			std::uint32_t mSubResourceIndex;
			D3D12_RESOURCE_STATES mBeforeState;
			std::optional<D3D12_RESOURCE_STATES> mAfterState;
			bool mAllowImplicitTransitions;
		};
	}
}

// ----------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		BarrierMergerStateContainer::BarrierMergerStateContainer(I_GPUResource& resource, const std::uint32_t subResourceIndex) :
			mResourcePtr(std::addressof(resource)),
			mSubResourceIndex(subResourceIndex),
			mBeforeState(resource.GetSubResourceState(subResourceIndex)),
			mAfterState(),
			mAllowImplicitTransitions(mBeforeState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON)
		{}

		I_GPUResource& BarrierMergerStateContainer::GetGPUResource()
		{
			return *mResourcePtr;
		}

		std::uint32_t BarrierMergerStateContainer::GetSubResourceIndex() const
		{
			return mSubResourceIndex;
		}

		bool BarrierMergerStateContainer::CanAfterStateBeCombined(const D3D12_RESOURCE_STATES requiredState) const
		{
			if (!mAfterState.has_value() || IsImplicitTransitionPossible(requiredState))
				return true;

			if (*mAfterState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON || requiredState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON)
				return (*mAfterState == requiredState);

			return Util::D3D12::IsResourceStateValid(*mAfterState | requiredState);
		}

		void BarrierMergerStateContainer::UpdateAfterState(const D3D12_RESOURCE_STATES requiredState)
		{
			assert(CanAfterStateBeCombined(requiredState) && "ERROR: The state provided to BarrierMergerStateContainer::UpdateAfterState() cannot be combined with the current after state!");
			
			// First, try to make an implicit transition.
			if (IsImplicitTransitionPossible(requiredState))
			{
				mBeforeState |= requiredState;
				return;
			}

			// If that failed, then we cannot allow any more implicit transitions until
			// the state goes back to the common state.
			mAllowImplicitTransitions = false;

			if (!mAfterState.has_value())
				mAfterState = requiredState;
			else
				*mAfterState |= requiredState;
		}

		bool BarrierMergerStateContainer::DoImplicitReadStateTransitionsAllowStateDecay() const
		{
			return (mAllowImplicitTransitions && (Util::D3D12::IsValidReadState(mBeforeState) || mBeforeState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON));
		}

		bool BarrierMergerStateContainer::IsImplicitTransitionPossible(const D3D12_RESOURCE_STATES requiredState) const
		{
			return (mAllowImplicitTransitions && Util::D3D12::IsImplicitStateTransitionPossible(*mResourcePtr, mBeforeState | requiredState));
		}

		bool BarrierMergerStateContainer::HasExplicitStateTransition() const
		{
			return (mAfterState.has_value() && *mAfterState != mBeforeState);
		}

		D3D12_RESOURCE_STATES BarrierMergerStateContainer::GetBeforeState() const
		{
			return mBeforeState;
		}

		D3D12_RESOURCE_STATES BarrierMergerStateContainer::GetAfterState() const
		{
			assert(HasExplicitStateTransition());
			return *mAfterState;
		}

		void BarrierMergerStateContainer::UpdateGPUResourceState()
		{
			mResourcePtr->SetSubResourceState(mBeforeState, mSubResourceIndex);
		}

		void BarrierMergerStateContainer::SwapStates()
		{
			if (mAfterState.has_value()) [[likely]]
			{
				mBeforeState = *mAfterState;
				mAfterState.reset();

				if (mBeforeState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON)
					mAllowImplicitTransitions = true;
			}
		}

		void BarrierMergerStateContainer::DecayResourceState()
		{
			mBeforeState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;
			mAfterState.reset();
			mAllowImplicitTransitions = true;
		}
	}
}