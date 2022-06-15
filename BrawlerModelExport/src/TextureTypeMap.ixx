module;
#include <assimp/material.h>
#include <DirectXTex.h>

export module Brawler.TextureTypeMap;
import Brawler.AssimpMaterials;
import Brawler.ModelTextureID;
import Brawler.NZStringView;

export namespace Brawler
{
	/// <summary>
	/// This is a constant-evaluated version of DirectX::IsCompressed() from DirectXTex.
	/// </summary>
	/// <returns>
	/// The function returns true if Format is a block-compressed texture format and
	/// false otherwise.
	/// </returns>
	template <DXGI_FORMAT Format>
	consteval bool IsBlockCompressedFormat()
	{
		switch (Format)
		{
		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
		case DXGI_FORMAT_BC2_TYPELESS:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM:
		case DXGI_FORMAT_BC5_TYPELESS:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM:
		case DXGI_FORMAT_BC6H_TYPELESS:
		case DXGI_FORMAT_BC6H_UF16:
		case DXGI_FORMAT_BC6H_SF16:
		case DXGI_FORMAT_BC7_TYPELESS:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB:
			return true;

		default:
			return false;
		}
	}
}

namespace Brawler
{
	namespace IMPL
	{
		template <
			DXGI_FORMAT IntermediateFormat,
			Brawler::AssimpMaterialKeyID MaterialKeyID
		>
			requires !Brawler::IsBlockCompressedFormat<IntermediateFormat>()
		struct AssimpTextureTypeMapInstantiation
		{
			static constexpr DXGI_FORMAT INTERMEDIATE_FORMAT = IntermediateFormat;
			static constexpr Brawler::AssimpMaterialKeyID MATERIAL_KEY_ID{ MaterialKeyID };
		};

		template <DXGI_FORMAT DesiredFormat>
		struct ModelTextureIDTypeMapInstantiation
		{
			static constexpr DXGI_FORMAT DESIRED_FORMAT = DesiredFormat;
		};
	}
}

export namespace Brawler
{
	template <aiTextureType TextureType>
	struct AssimpTextureTypeMap
	{
		static_assert(sizeof(TextureType) != sizeof(TextureType), "ERROR: An explicit instantiation for Brawler::AssimpTextureTypeMap for a particular aiTextureType was never provided! (See TextureTypeMap.ixx.)");
	};

	template <>
	struct AssimpTextureTypeMap<aiTextureType::aiTextureType_DIFFUSE> : public IMPL::AssimpTextureTypeMapInstantiation<
		DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		AssimpMaterialKeyID::COLOR_DIFFUSE
	>
	{};
}

export namespace Brawler
{
	template <ModelTextureID TextureType>
	struct ModelTextureIDTypeMap
	{
		static_assert(sizeof(TextureType) != sizeof(TextureType), "ERROR: An explicit instantiation for Brawler::ModelTextureIDTypeMap for a particular aiTextureType was never provided! (See TextureTypeMap.ixx.)");
	};

	template <>
	struct ModelTextureIDTypeMap<ModelTextureID::DIFFUSE_ALBEDO> : public IMPL::ModelTextureIDTypeMapInstantiation<DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM_SRGB>
	{
		static constexpr Brawler::NZWStringView TEXTURE_NAME_FORMAT_STR{ L"LOD{}_{}_DiffuseAlbedo" };
	};
}

export namespace Brawler
{
	template <aiTextureType TextureType>
	consteval DXGI_FORMAT GetIntermediateTextureFormat()
	{
		static_assert(!IsBlockCompressedFormat<AssimpTextureTypeMap<TextureType>::INTERMEDIATE_FORMAT>(), "ERROR: Intermediate texture formats cannot be block compressed, because many DirectXTex functions do not work directly with block-compressed formats, and compression/decompression on the CPU can be incredibly slow.");
		
		return AssimpTextureTypeMap<TextureType>::INTERMEDIATE_FORMAT;
	}

	template <aiTextureType TextureType>
	consteval AssimpMaterialKeyID GetTextureMaterialKeyID()
	{
		return AssimpTextureTypeMap<TextureType>::MATERIAL_KEY_ID;
	}

	template <ModelTextureID TextureType>
	consteval DXGI_FORMAT GetDesiredTextureFormat()
	{
		return ModelTextureIDTypeMap<TextureType>::DESIRED_FORMAT;
	}

	template <ModelTextureID TextureType>
	consteval NZWStringView GetTextureNameFormatString()
	{
		return ModelTextureIDTypeMap<TextureType>::TEXTURE_NAME_FORMAT_STR;
	}
}