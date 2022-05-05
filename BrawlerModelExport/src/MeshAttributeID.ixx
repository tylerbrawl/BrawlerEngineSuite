module;
#include <cstdint>

export module Brawler.MeshAttributeID;

export namespace Brawler
{
	enum class MeshAttributeID : std::uint32_t
	{
		STATIC_VERTEX_BUFFER_ATTRIBUTE,
		OPAQUE_MATERIAL_DEFINITION_ATTRIBUTE,
		
		COUNT_OR_ERROR
	};
}