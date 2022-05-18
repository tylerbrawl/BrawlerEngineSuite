module;
#include <tuple>
#include <optional>
#include <assimp/material.h>

export module Brawler.AssimpMaterialKey;
export import Brawler.AssimpMaterialKeyID;

// This module represents my attempt to de-crapify the material property system of Assimp.

namespace Brawler
{
	template <AssimpMaterialKeyID KeyID>
	struct MaterialKeyInfo
	{
		static_assert(sizeof(KeyID) != sizeof(KeyID), "ERROR: An explicit template specialization for Brawler::MaterialKeyInfo was never specified for a given Brawler::AssimpMaterialKeyID! (See AssimpMaterialKey.ixx.)");
	};

	// Information regarding the data types and keys can be found at http://assimp.sourceforge.net/lib_html/materials.html.

	using MaterialPropertyTuple = std::tuple<const char*, std::uint32_t, std::uint32_t>;

	template <typename PropertyDataType_>
	struct MaterialKeyInfoInstantiation
	{
		using PropertyDataType = PropertyDataType_;
	};

	template <>
	struct MaterialKeyInfo<AssimpMaterialKeyID::NAME> : public MaterialKeyInfoInstantiation<aiString>
	{
		static constexpr MaterialPropertyTuple PROPERTY_TUPLE{ AI_MATKEY_NAME };
	};

	template <>
	struct MaterialKeyInfo<AssimpMaterialKeyID::COLOR_DIFFUSE> : public MaterialKeyInfoInstantiation<aiColor3D>
	{
		static constexpr MaterialPropertyTuple PROPERTY_TUPLE{ AI_MATKEY_COLOR_DIFFUSE };
	};

	template <>
	struct MaterialKeyInfo<AssimpMaterialKeyID::OPACITY> : public MaterialKeyInfoInstantiation<float>
	{
		static constexpr MaterialPropertyTuple PROPERTY_TUPLE{ AI_MATKEY_OPACITY };
	};
}

export namespace Brawler
{
	template <AssimpMaterialKeyID KeyID>
	__forceinline auto GetAssimpMaterialProperty(const aiMaterial& material)
	{
		using PropertyDataType = typename MaterialKeyInfo<KeyID>::PropertyDataType;

		PropertyDataType propertyData{};
		const aiReturn getResult{ material.Get(
			std::get<0>(MaterialKeyInfo<KeyID>::PROPERTY_TUPLE),
			std::get<1>(MaterialKeyInfo<KeyID>::PROPERTY_TUPLE),
			std::get<2>(MaterialKeyInfo<KeyID>::PROPERTY_TUPLE),
			propertyData
		) };

		if (getResult != aiReturn::aiReturn_SUCCESS) [[unlikely]]
			return std::optional<PropertyDataType>{};

		return std::optional<PropertyDataType>{ std::move(propertyData) };
	}
}