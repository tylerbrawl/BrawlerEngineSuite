module;
#include <tuple>

export module Brawler.ResourceStateZoneOptimizerStateTraits;
import Brawler.PolymorphismInfo;
import Brawler.D3D12.I_ResourceStateZoneOptimizerState;
import Brawler.D3D12.IgnoreResourceStateZoneOptimizerState;
import Brawler.D3D12.ReadResourceStateZoneOptimizerState;
import Brawler.D3D12.ResourceStateZoneOptimizerStateID;

namespace Brawler
{
	template <D3D12::ResourceStateZoneOptimizerStateID StateID>
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
	struct StateMap<D3D12::ResourceStateZoneOptimizerStateID::READ_RESOURCE_STATE_ZONE> : public StateMapInstantiation<D3D12::ReadResourceStateZoneOptimizerState>
	{};

	template <>
	struct StateMap<D3D12::ResourceStateZoneOptimizerStateID::IGNORE_RESOURCE_STATE_ZONE> : public StateMapInstantiation<D3D12::IgnoreResourceStateZoneOptimizerState>
	{};

	// Since at least one of the types used as a StateType does not have a default constructor,
	// we need to use this helper class to create a value which can be "converted" to a given
	// type.
	class TrivialTypeSolver
	{
	public:
		TrivialTypeSolver() = default;

		template <typename T>
		operator T();
	};

	template <D3D12::ResourceStateZoneOptimizerStateID CurrID>
	consteval auto CreateStateTuple()
	{
		constexpr D3D12::ResourceStateZoneOptimizerStateID NEXT_ID = static_cast<D3D12::ResourceStateZoneOptimizerStateID>(std::to_underlying(CurrID) + 1);

		if constexpr (NEXT_ID != D3D12::ResourceStateZoneOptimizerStateID::COUNT_OR_ERROR)
			return std::tuple_cat(std::tuple<typename StateMap<CurrID>::StateType>{ TrivialTypeSolver{} }, CreateStateTuple<NEXT_ID>());
		else
			return std::tuple<typename StateMap<CurrID>::StateType>{ TrivialTypeSolver{} };
	}
}

export namespace Brawler
{
	template <typename DummyType>
	struct PolymorphismInfo<D3D12::I_ResourceStateZoneOptimizerState<DummyType>> : public PolymorphismInfoInstantiation<D3D12::ResourceStateZoneOptimizerStateID, decltype(CreateStateTuple<static_cast<D3D12::ResourceStateZoneOptimizerStateID>(0)>())>
	{};
}