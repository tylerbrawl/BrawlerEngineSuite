module;
#include <concepts>
#include <vector>

export module Brawler.ComponentID;

export namespace Brawler
{
	// NOTE: The order in which ComponentIDs are listed here determines the order in which
	// they are updated! All components for a SceneNode are updated sequentially, but
	// multiple SceneNodes and their respective components are updated concurrently independent
	// of each other.

	enum class ComponentID
	{
		// Add additional ComponentIDs here. Then, provide a static mapping by adding
		// explicit template instantiations of IMPL::ComponentIDMap and
		// IMPL::ComponentTypeMap in ComponentMap.ixx.
		TRANSFORM_COMPONENT,
		VIEW_COMPONENT,

		COUNT_OR_ERROR
	};
}