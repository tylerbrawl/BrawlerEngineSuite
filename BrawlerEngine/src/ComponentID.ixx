module;
#include <concepts>
#include <vector>

export module Brawler.ComponentID;

export namespace Brawler
{
	enum class ComponentID
	{
		// Add additional ComponentIDs here. Then, provide a static mapping by adding
		// explicit template instantiations of IMPL::ComponentIDMap and
		// IMPL::ComponentTypeMap in ComponentMap.ixx.
		VIEW_COMPONENT,

		COUNT_OR_ERROR
	};
}