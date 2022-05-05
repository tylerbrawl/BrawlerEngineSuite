module;
#include <concepts>

export module Brawler.SceneNodeID;
import Brawler.SceneNode;

export namespace Brawler
{
	enum class SceneNodeID
	{
		COUNT_OR_ERROR
	};
}

namespace IMPL
{
	template <typename T>
		requires std::derived_from<T, Brawler::SceneNode>
	struct SceneNodeIDMap
	{
		static_assert(sizeof(T) != sizeof(T), "ERROR: A class which was derived from SceneNode was never assigned a static SceneNodeID mapping! (See IMPL::SceneNodeIDMap in SceneNodeID.ixx.)");

		static constexpr Brawler::SceneNodeID ID = Brawler::SceneNodeID::COUNT_OR_ERROR;
	};

	// --------------------------------------------------------------------------------

	template <Brawler::SceneNodeID ID>
	struct SceneNodeTypeMap
	{
		static_assert(sizeof(ID) != sizeof(ID), "ERROR: A SceneNodeID was never assigned a static mapping to a class derived from SceneNode! (See IMPL::SceneNodeTypeMap in SceneNodeID.ixx.)");

		using Type = void;
	};
}

export namespace Brawler
{
	template <typename T>
		requires std::derived_from<T, Brawler::SceneNode>
	constexpr SceneNodeID GetSceneNodeID()
	{
		return IMPL::SceneNodeIDMap<T>::ID;
	}

	template <Brawler::SceneNodeID ID>
	using SceneNodeType = IMPL::SceneNodeTypeMap<ID>::Type;
}