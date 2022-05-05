module;
#include <cstdint>
#include <type_traits>

export module Brawler.SerializedMeshAttribute;
import Brawler.MeshAttributeID;

export namespace Brawler
{
	class OpaqueMaterialDefinitionAttribute;
}

export namespace Brawler
{
	template <typename T>
	struct SerializedMeshAttribute
	{
		static_assert(sizeof(T) != sizeof(T), "ERROR: An explicit template specialization of SerializedMeshAttribute was never provided for this type! (See SerializedMeshAttribute.ixx for more details.)");
	};

	template <>
	struct SerializedMeshAttribute<OpaqueMaterialDefinitionAttribute>
	{
		// For right now, the only thing which will define the material is a diffuse albedo
		// texture. I need to restrain myself to take things one small step at a time.

		std::uint64_t AlbedoTexturePathHash;
	};
}