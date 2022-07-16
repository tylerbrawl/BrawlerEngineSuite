module;
#include <concepts>
#include <vector>
#include <array>

export module Brawler.ComponentMap;
import Brawler.Components;

namespace IMPL
{
	// Mapping from I_Component Derived Class to ComponentID...

	template <typename T>
		requires std::derived_from<T, Brawler::I_Component>
	struct ComponentIDMap
	{
		static constexpr Brawler::ComponentID ID = Brawler::ComponentID::COUNT_OR_ERROR;
	};

	template <Brawler::ComponentID MappedID>
	struct ComponentIDMapInstantiation
	{
		static constexpr Brawler::ComponentID ID = MappedID;
	};

	template <>
	struct ComponentIDMap<Brawler::TransformComponent> final : public ComponentIDMapInstantiation<Brawler::ComponentID::TRANSFORM_COMPONENT>
	{};

	template <>
	struct ComponentIDMap<Brawler::ViewComponent> final : public ComponentIDMapInstantiation<Brawler::ComponentID::VIEW_COMPONENT>
	{};

	// --------------------------------------------------------------------------------

	// Mapping from ComponentID to I_Component Derived Class...

	template <Brawler::ComponentID ID>
	struct ComponentTypeMap
	{
		static_assert(sizeof(ID) != sizeof(ID), "ERROR: A static mapping from ComponentID to a derived I_Component class was never provided! (See IMPL::ComponentTypeMap in ComponentMap.ixx.)");

		using Type = void;
	};

	template <typename MappedType>
		requires std::derived_from<MappedType, Brawler::I_Component>
	struct ComponentTypeMapInstantiation
	{
		using Type = MappedType;
	};

	template <>
	struct ComponentTypeMap<Brawler::ComponentID::TRANSFORM_COMPONENT> final : public ComponentTypeMapInstantiation<Brawler::TransformComponent>
	{};

	template <>
	struct ComponentTypeMap<Brawler::ComponentID::VIEW_COMPONENT> final : public ComponentTypeMapInstantiation<Brawler::ViewComponent>
	{};

	// ---------------------------------------------------------------------------------

	template <typename BaseType, Brawler::ComponentID CurrentID, std::size_t ArraySize>
	consteval void GetCompatibleComponentIDsIMPL(std::array<Brawler::ComponentID, ArraySize>& componentIDArr, std::size_t currIndex)
	{
		constexpr Brawler::ComponentID NEXT_ID{ static_cast<Brawler::ComponentID>(std::to_underlying(CurrentID) + 1) };
		
		if constexpr (NEXT_ID != Brawler::ComponentID::COUNT_OR_ERROR)
		{
			using CorrespondingType = ComponentTypeMap<CurrentID>::Type;

			if constexpr (std::derived_from<CorrespondingType, BaseType> || std::is_same_v<CorrespondingType, BaseType>)
				componentIDArr[currIndex++] = CurrentID;

			GetCompatibleComponentIDsIMPL<BaseType, NEXT_ID>(componentIDArr, currIndex);
		}
	}

	template <typename BaseType, Brawler::ComponentID CurrentID>
	consteval std::size_t GetCompatibleComponentIDCount()
	{
		using CorrespondingType = ComponentTypeMap<CurrentID>::Type;

		constexpr std::size_t CURRENT_COUNT_VALUE = ((std::derived_from<CorrespondingType, BaseType> || std::is_same_v<CorrespondingType, BaseType>) ? 1 : 0);
		constexpr Brawler::ComponentID NEXT_ID = static_cast<Brawler::ComponentID>(std::to_underlying(CurrentID) + 1);

		if constexpr (NEXT_ID != Brawler::ComponentID::COUNT_OR_ERROR)
			return CURRENT_COUNT_VALUE + GetCompatibleComponentIDCount<BaseType, NEXT_ID>();
		else
			return CURRENT_COUNT_VALUE;
	}
}

export namespace Brawler
{
	template <typename T>
		requires std::derived_from<T, Brawler::I_Component>
	constexpr ComponentID GetComponentID()
	{
		return IMPL::ComponentIDMap<T>::ID;
	}

	template <Brawler::ComponentID ID>
	using ComponentType = IMPL::ComponentTypeMap<ID>::Type;

	/// <summary>
	/// Retrieves a compile-time std::vector of ComponentIDs which correspond to derived classes
	/// of the type specified by the template parameter T. (This includes the ComponentID which
	/// corresponds to T itself.)
	/// </summary>
	/// <typeparam name="T">
	/// - The base type which is used to determine compatibility.
	/// </typeparam>
	/// <returns>
	/// The function returns a compile-time std::vector of all ComponentIDs which correspond to
	/// derived classes of the type specified by the template parameter T.
	/// </returns>
	template <typename T>
		requires std::derived_from<T, Brawler::I_Component> && !std::is_same_v<T, Brawler::I_Component>
	consteval auto GetCompatibleComponentIDs()
	{
		constexpr Brawler::ComponentID FIRST_ID = static_cast<Brawler::ComponentID>(0);
		constexpr std::size_t COMPATIBLE_COMPONENT_ID_COUNT = IMPL::GetCompatibleComponentIDCount<T, FIRST_ID>();

		std::array<Brawler::ComponentID, COMPATIBLE_COMPONENT_ID_COUNT> componentIDArr{};
		std::size_t workingIndexValue = 0;

		IMPL::GetCompatibleComponentIDsIMPL<T, static_cast<ComponentID>(0)>(componentIDArr, workingIndexValue);

		return componentIDArr;
	}
}