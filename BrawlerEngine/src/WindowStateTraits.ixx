module;
#include <tuple>
#include <optional>

export module Brawler.WindowStateTraits;
import Brawler.PolymorphismInfo;
import Brawler.I_WindowState;
import Brawler.WindowedModeWindowState;
import Brawler.BorderlessWindowedModeWindowState;
import Brawler.FullscreenModeWindowState;
import Brawler.WindowStateID;

namespace Brawler
{
	template <WindowStateID StateID>
	struct StateMap
	{
		static_assert(sizeof(StateID) != sizeof(StateID));
	};

	template <typename T>
	struct StateMapInstantiation
	{
		using Type = T;
	};

	template <>
	struct StateMap<WindowStateID::WINDOWED> : public StateMapInstantiation<WindowedModeWindowState>
	{};

	template <>
	struct StateMap<WindowStateID::BORDERLESS_WINDOWED_FULL_SCREEN> : public StateMapInstantiation<BorderlessWindowedModeWindowState>
	{};

	template <>
	struct StateMap<WindowStateID::FULL_SCREEN> : public StateMapInstantiation<FullscreenModeWindowState>
	{};

	template <WindowStateID StateID>
	consteval auto CreateStateTuple()
	{
		using CurrStateType = typename StateMap<StateID>::Type;
		constexpr WindowStateID NEXT_STATE_ID = static_cast<WindowStateID>(std::to_underlying(StateID) + 1);

		if constexpr (NEXT_STATE_ID != WindowStateID::COUNT_OR_ERROR)
			return std::tuple_cat(std::tuple<std::optional<CurrStateType>>{}, CreateStateTuple<NEXT_STATE_ID>());
		else
			return std::tuple<std::optional<CurrStateType>>{};
	}

	template <typename T>
	struct OptionalTupleSolver
	{};

	template <typename... StateTypes>
	struct OptionalTupleSolver<std::tuple<std::optional<StateTypes>...>>
	{
		using TupleType = std::tuple<std::decay_t<StateTypes>...>;
	};
}

export namespace Brawler
{
	template <typename DummyType>
	struct PolymorphismInfo<I_WindowState<DummyType>> : public PolymorphismInfoInstantiation<WindowStateID, typename OptionalTupleSolver<decltype(CreateStateTuple<static_cast<WindowStateID>(0)>())>::TupleType>
	{};
}