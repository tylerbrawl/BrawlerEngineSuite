module;
#include <concepts>
#include <memory>
#include <array>
#include <cassert>

export module Brawler.ApplicationStateMap;
import Brawler.ApplicationStateID;
import Brawler.I_ApplicationState;

// Rather than importing the individual state types individually, just add
// them to the following module (see ApplicationStates.ixx):
import Brawler.ApplicationStates;

export namespace Brawler
{
	class ApplicationStateStack;
}

namespace Brawler
{
	namespace IMPL
	{
		template <typename T>
			requires std::derived_from<T, Brawler::I_ApplicationState>
		struct ApplicationStateIDMap
		{
			static_assert(sizeof(T) != sizeof(T), "ERROR: A static mapping from a derived class of I_ApplicationState to an ApplicationStateID was never defined! (See Brawler::IMPL::ApplicationStateIDMap in ApplicationStateMap.ixx.)");

			static constexpr Brawler::ApplicationStateID STATE_ID = Brawler::ApplicationStateID::COUNT_OR_ERROR;
		};

		template <>
		struct ApplicationStateIDMap<Brawler::GameState>
		{
			static constexpr Brawler::ApplicationStateID STATE_ID = Brawler::ApplicationStateID::GAME;
		};

		// ------------------------------------------------------------------------------------------------

		template <Brawler::ApplicationStateID ID>
		struct ApplicationStateTypeMap
		{
			static_assert(sizeof(ID) != sizeof(ID), "ERROR: A static mapping from an ApplicationStateID to a derived class of I_ApplicationState was never defined! (See Brawler::IMPL::ApplicationStateTypeMap in ApplicationStateMap.ixx.)");

			using Type = void;
		};

		template <>
		struct ApplicationStateTypeMap<Brawler::ApplicationStateID::GAME>
		{
			using Type = Brawler::GameState;
		};

		// -------------------------------------------------------------------------------------------------

		template <Brawler::ApplicationStateID ID>
		std::unique_ptr<Brawler::I_ApplicationState> CreateApplicationState(ApplicationStateStack& stateStack)
		{
			using StateType = ApplicationStateTypeMap<ID>::Type;
			static_assert(std::derived_from<StateType, Brawler::I_ApplicationState>, "ERROR: An ApplicationStateID was mapped to a type which was not a derived class of I_ApplicationState! (See Brawler::IMPL::ApplicationStateTypeMap in ApplicationStateMap.ixx.)");

			return std::make_unique<StateType>(stateStack);
		}

		static constexpr std::array<Brawler::FunctionPtr<std::unique_ptr<Brawler::I_ApplicationState>, Brawler::ApplicationStateStack&>, std::to_underlying(Brawler::ApplicationStateID::COUNT_OR_ERROR)> STATE_CREATION_FUNCTION_PTR_ARR{
			// Add template instantiations of CreateApplicationState() here.
			CreateApplicationState<Brawler::ApplicationStateID::GAME>
		};
	}
}

export namespace Brawler
{
	template <typename T>
		requires std::derived_from<T, I_ApplicationState>
	constexpr ApplicationStateID GetApplicationStateID()
	{
		return IMPL::ApplicationStateIDMap<T>::STATE_ID;
	}

	template <Brawler::ApplicationStateID ID>
	using ApplicationStateType = IMPL::ApplicationStateTypeMap<ID>::Type;

	std::unique_ptr<I_ApplicationState> CreateApplicationState(const ApplicationStateID stateID, ApplicationStateStack& stateStack)
	{
		assert(stateID != ApplicationStateID::COUNT_OR_ERROR && "ERROR: An invalid ApplicationStateID was specified for Brawler::CreateApplicationState()!");

		return IMPL::STATE_CREATION_FUNCTION_PTR_ARR[std::to_underlying(stateID)](stateStack);
	}
}