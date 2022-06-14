module;
#include <cstddef>

export module Brawler.SerializedMaterialDefinition;
import Brawler.MaterialID;

export namespace Brawler
{
#pragma pack(push)
#pragma pack(1)
	struct SerializedMaterialDefinition
	{
		MaterialID Identifier;
		
		std::size_t DiffuseAlbedoTextureHash;
	};
#pragma pack(pop)
}