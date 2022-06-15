module;
#include <optional>
#include <assimp/material.h>

export module Brawler.AssimpTextureConverter:AssimpTextureMaterialColorMap;
export import Brawler.AssimpMaterials;

namespace Brawler
{
	namespace IMPL
	{
		template <aiTextureType TextureType>
		struct MaterialColorMap
		{
			static constexpr bool HAS_MATERIAL_COLOR = false;
		};

		template <Brawler::AssimpMaterialKeyID MaterialKeyID>
		struct MaterialColorMapInstantiation
		{
			static constexpr bool HAS_MATERIAL_COLOR = true;
			static constexpr Brawler::AssimpMaterialKeyID MATERIAL_KEY_ID = MaterialKeyID;
		};

		template <>
		struct MaterialColorMap<aiTextureType::aiTextureType_DIFFUSE> : public MaterialColorMapInstantiation<Brawler::AssimpMaterialKeyID::COLOR_DIFFUSE>
		{};
	}
}

export namespace Brawler
{
	template <aiTextureType TextureType>
	consteval std::optional<AssimpMaterialKeyID> GetMaterialColor()
	{
		if constexpr (IMPL::MaterialColorMap<TextureType>::HAS_MATERIAL_COLOR)
			return std::optional<AssimpMaterialKeyID>{ IMPL::MaterialColorMap<TextureType>::MATERIAL_KEY_ID };
		else
			return std::optional<AssimpMaterialKeyID>{};
	}
}