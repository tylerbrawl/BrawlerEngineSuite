module;
#include <tuple>

export module Brawler.GPUResourceStateTrackerStateTraits;
import Brawler.PolymorphismInfo;
import Brawler.D3D12.GPUResourceStateTrackerStateID;
import Brawler.D3D12.I_GPUResourceStateTrackerState;
import Brawler.D3D12.ExplicitBarrierGPUResourceStateTrackerState;
import Brawler.D3D12.ImplicitBarrierGPUResourceStateTrackerState;

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
	struct StateMap<D3D12::GPUResourceStateTrackerStateID::EXPLICIT_BARRIER> : public StateMapInstantiation<D3D12::ExplicitBarrierGPUResourceStateTrackerState>
	{};

	template <>
	struct StateMap<D3D12::GPUResourceStateTrackerStateID::IMPLICIT_BARRIER> : public StateMapInstantiation<D3D12::ImplicitBarrierGPUResourceStateTrackerState>
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