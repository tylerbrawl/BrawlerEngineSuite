module;
#include <concepts>
#include <vector>

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
	struct ComponentIDMap<Brawler::I_ViewComponent> final : public ComponentIDMapInstantiation<Brawler::ComponentID::VIEW_COMPONENT>
	{};

	// --------------------------------------------------------------------------------

	// Mapping from ComponentID to I_Component Derived Class...

	template <Brawler::ComponentID ID>
	struct ComponentTypeMap
	{
		static_assert(sizeof(ID) != sizeof(ID), "ERROR: A static mapping from ComponentID to a derived I_Component class was never provided! (See IMPL::ComponentTypeMap in ComponentID.ixx.)");

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
	struct ComponentTypeMap<Brawler::ComponentID::VIEW_COMPONENT> final : public ComponentTypeMapInstantiation<Brawler::I_ViewComponent>
	{};

	// ---------------------------------------------------------------------------------

	template <typename BaseType, Brawler::ComponentID CurrentID>
	constexpr void GetCompatibleComponentIDsIMPL(std::vector<Brawler::ComponentID>& componentIDArr)
	{
		if constexpr (CurrentID == Brawler::ComponentID::COUNT_OR_ERROR)
			return;

		using CorrespondingType = ComponentTypeMap<CurrentID>::Type;

		if constexpr (std::derived_from<CorrespondingType, BaseType> || std::is_same_v<CorrespondingType, BaseType>)
			componentIDArr.push_back(CurrentID);

		constexpr Brawler::ComponentID NEXT_ID{ static_cast<Brawler::ComponentID>(std::to_underlying(CurrentID) + 1) };
		GetCompatibleComponentIDsIMPL<BaseType, NEXT_ID>(componentIDArr);
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
	consteval std::vector<ComponentID> GetCompatibleComponentIDs()
	{
		std::vector<ComponentID> componentIDArr{};
		IMPL::GetCompatibleComponentIDsIMPL<T, static_cast<ComponentID>(0)>(componentIDArr);

		return componentIDArr;
	}
}