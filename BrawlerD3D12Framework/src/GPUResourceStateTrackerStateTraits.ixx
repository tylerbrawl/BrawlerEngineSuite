module;
#include <tuple>

export module Brawler.GPUResourceStateTrackerStateTraits;
import Brawler.PolymorphismInfo;
import Brawler.D3D12.GPUResourceStateTrackerStateID;
import Brawler.D3D12.I_GPUResourceStateTrackerState;
import Brawler.D3D12.GPUResourceSpecialInitializationState;
import Brawler.D3D12.GPUResourceBarrierTypeSelectorState;
import Brawler.D3D12.ImmediateGPUResourceBarrierState;
import Brawler.D3D12.SplitGPUResourceBarrierState;

namespace Brawler
{
	template <D3D12::GPUResourceStateTrackerStateID StateID>
	struct StateMap
	{
		static_assert(sizeof(StateID) != sizeof(StateID));
	};

	template <typename StateType_>
	struct StateMapInstantiation
	{
		using StateType = StateType_;
	};

	template <>
	struct StateMap<D3D12::GPUResourceStateTrackerStateID::GPU_RESOURCE_SPECIAL_INITIALIZATION> : public StateMapInstantiation<D3D12::GPUResourceSpecialInitializationState>
	{};

	template <>
	struct StateMap<D3D12::GPUResourceStateTrackerStateID::BARRIER_TYPE_SELECTOR> : public StateMapInstantiation<D3D12::GPUResourceBarrierTypeSelectorState>
	{};

	template <>
	struct StateMap<D3D12::GPUResourceStateTrackerStateID::IMMEDIATE_BARRIER> : public StateMapInstantiation<D3D12::ImmediateGPUResourceBarrierState>
	{};

	template <>
	struct StateMap<D3D12::GPUResourceStateTrackerStateID::SPLIT_BARRIER> : public StateMapInstantiation<D3D12::SplitGPUResourceBarrierState>
	{};

	template <D3D12::GPUResourceStateTrackerStateID StateID>
	consteval auto CreateStateTuple()
	{
		constexpr D3D12::GPUResourceStateTrackerStateID NEXT_ID = static_cast<D3D12::GPUResourceStateTrackerStateID>(std::to_underlying(StateID) + 1);

		if constexpr (NEXT_ID != D3D12::GPUResourceStateTrackerStateID::COUNT_OR_ERROR)
			return std::tuple_cat(std::tuple<typename StateMap<StateID>::StateType>{}, CreateStateTuple<NEXT_ID>());
		else
			return std::tuple<typename StateMap<StateID>::StateType>{};
	}
}

// -------------------------------------------------------------------------------------------------------------------------------

export namespace Brawler
{
	template <typename DummyType>
	struct PolymorphismInfo<D3D12::I_GPUResourceStateTrackerState<DummyType>> : public PolymorphismInfoInstantiation<D3D12::GPUResourceStateTrackerStateID, decltype(CreateStateTuple<static_cast<D3D12::GPUResourceStateTrackerStateID>(0)>())>
	{};
}