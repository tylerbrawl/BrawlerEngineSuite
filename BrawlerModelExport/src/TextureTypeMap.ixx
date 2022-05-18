module;
#include <assimp/material.h>
#include <dxgiformat.h>
#include <DirectXTex.h>

export module Brawler.TextureTypeMap;
import Brawler.MipMapGeneration;
import Brawler.AssimpMaterialKeyID;

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
		template <typename MipMapGeneratorType>
		concept IsMipMapGenerator = requires (MipMapGeneratorType generator, const DirectX::ScratchImage& srcTexture)
		{
			// void MipMapGeneratorType::Update(const DirectX::ScratchImage& srcTexture)
			{ generator.Update(srcTexture) } -> std::same_as<void>;

			// bool-ish MipMapGeneratorType::IsMipMapGenerationFinished() const
			{ std::as_const(generator).IsMipMapGenerationFinished() } -> std::convertible_to<bool>;

			// DirectX::ScratchImage MipMapGeneratorType::ExtractGeneratedMipMaps()
			{ generator.ExtractGeneratedMipMaps() } -> std::same_as<DirectX::ScratchImage>;
		};
		
		template <
			DXGI_FORMAT IntermediateFormat, 
			DXGI_FORMAT DesiredFormat,
			Brawler::AssimpMaterialKeyID MaterialKeyID,
			typename MipMapGeneratorType_
		>
			requires !Brawler::IsBlockCompressedFormat<IntermediateFormat>() && IsMipMapGenerator<MipMapGeneratorType_>
		struct TextureTypeMapInstantiation
		{
			static constexpr DXGI_FORMAT INTERMEDIATE_FORMAT = IntermediateFormat;
			static constexpr DXGI_FORMAT DESIRED_FORMAT = DesiredFormat;
			static constexpr Brawler::AssimpMaterialKeyID MATERIAL_KEY_ID{ MaterialKeyID };

			using MipMapGeneratorType = MipMapGeneratorType_;
		};
	}
}

export namespace Brawler
{
	template <aiTextureType TextureType>
	struct TextureTypeMap
	{
		static_assert(sizeof(TextureType) != sizeof(TextureType), "ERROR: An explicit instantiation for Brawler::TextureTypeMap for a particular aiTextureType was never provided! (See TextureTypeMap.ixx.)");
	};

	template <>
	struct TextureTypeMap<aiTextureType::aiTextureType_DIFFUSE> : public IMPL::TextureTypeMapInstantiation<
		DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM_SRGB,
		AssimpMaterialKeyID::COLOR_DIFFUSE,

		// Albedo textures *CAN* have mip-maps created in the default fashion.
		GenericMipMapGenerator
	>
	{};
}

export namespace Brawler
{
	template <aiTextureType TextureType>
	consteval DXGI_FORMAT GetIntermediateTextureFormat()
	{
		static_assert(!IsBlockCompressedFormat<TextureTypeMap<TextureType>::INTERMEDIATE_FORMAT>(), "ERROR: Intermediate texture formats cannot be block compressed, because many DirectXTex functions do not work directly with block-compressed formats, and compression/decompression on the CPU can be incredibly slow.");
		
		return TextureTypeMap<TextureType>::INTERMEDIATE_FORMAT;
	}
	
	template <aiTextureType TextureType>
	consteval DXGI_FORMAT GetDesiredTextureFormat()
	{
		return TextureTypeMap<TextureType>::DESIRED_FORMAT;
	}

	template <aiTextureType TextureType>
	consteval AssimpMaterialKeyID GetTextureMaterialKeyID()
	{
		return TextureTypeMap<TextureType>::MATERIAL_KEY_ID;
	}

	template <aiTextureType TextureType>
	using ModelTextureMipMapGeneratorType = typename TextureTypeMap<TextureType>::MipMapGeneratorType;
}