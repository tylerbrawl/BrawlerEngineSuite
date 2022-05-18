module;
#include <optional>
#include <filesystem>
#include <string_view>
#include <cassert>
#include <format>
#include <stdexcept>
#include <array>
#include <assimp/material.h>
#include <DirectXTex.h>

export module Brawler.MaterialColorModelTextureBuilder;
import Brawler.I_ModelTextureBuilder;
import Brawler.ImportedMesh;
import Brawler.AssimpMaterialKey;
import Util.General;
import Brawler.TextureTypeMap;

#pragma push_macro("max")
#undef max

namespace Brawler
{
	template <aiTextureType TextureType>
	concept HasMaterialKeyID = (Brawler::GetTextureMaterialKeyID<TextureType>() != Brawler::AssimpMaterialKeyID::COUNT_OR_ERROR);
}

export namespace Brawler
{
	template <aiTextureType TextureType>
		requires HasMaterialKeyID<TextureType>
	class MaterialColorModelTextureBuilder final : public I_ModelTextureBuilder<TextureType>
	{
	public:
		explicit MaterialColorModelTextureBuilder(const ImportedMesh& mesh);

		MaterialColorModelTextureBuilder(const MaterialColorModelTextureBuilder& rhs) = delete;
		MaterialColorModelTextureBuilder& operator=(const MaterialColorModelTextureBuilder& rhs) = delete;

		MaterialColorModelTextureBuilder(MaterialColorModelTextureBuilder&& rhs) noexcept = default;
		MaterialColorModelTextureBuilder& operator=(MaterialColorModelTextureBuilder&& rhs) noexcept = default;

		DirectX::ScratchImage CreateIntermediateScratchTexture() const override;
		std::wstring_view GetUniqueTextureName() const override;

	private:
		std::wstring mUniqueTextureName;
		aiColor3D mRawMaterialColor;
		float mAlphaValue;
	};
}

// ----------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <aiTextureType TextureType>
	struct MaterialColorInfo
	{
		static_assert(sizeof(TextureType) != sizeof(TextureType));
	};

	template <>
	struct MaterialColorInfo<aiTextureType::aiTextureType_DIFFUSE>
	{
		static constexpr std::string_view UNIQUE_TEXTURE_TYPE_STR{ "DIFFUSE" };
		static constexpr std::wstring_view UNIQUE_TEXTURE_TYPE_WSTR{ L"DIFFUSE" };

		static __forceinline float GetMaterialColorAlphaValue(const aiMaterial& material)
		{
			const std::optional<float> opacity{ Brawler::GetAssimpMaterialProperty<AssimpMaterialKeyID::OPACITY>(material) };
			return (opacity.has_value() ? *opacity : 1.0f);
		}
	};
}

namespace Brawler
{
	template <aiTextureType TextureType>
		requires HasMaterialKeyID<TextureType>
	MaterialColorModelTextureBuilder<TextureType>::MaterialColorModelTextureBuilder(const ImportedMesh& mesh) :
		I_ModelTextureBuilder<TextureType>(),
		mUniqueTextureName(),
		mRawMaterialColor(),
		mAlphaValue(1.0f)
	{
		const aiMaterial& material{ mesh.GetMeshMaterial() };
		const std::optional<aiString> materialName{ Brawler::GetAssimpMaterialProperty<AssimpMaterialKeyID::NAME>(material) };

		if (!materialName.has_value()) [[unlikely]]
			throw std::runtime_error{ "ERROR: An attempt was made to create a MaterialColorModelTextureBuilder for a raw material color, but a unique texture name could not be created because the source material has no name!" };

		constexpr Brawler::AssimpMaterialKeyID MATERIAL_KEY_ID = Brawler::GetTextureMaterialKeyID<TextureType>();
		const std::optional<aiColor3D> materialColor{ Brawler::GetAssimpMaterialProperty<MATERIAL_KEY_ID>(material) };

		if (!materialColor.has_value()) [[unlikely]]
			throw std::runtime_error{ std::format("ERROR: An attempt was made to get a raw {} material color from the material {} for a MaterialColorModelTextureBuilder instance, but this material color did not exist!", MaterialColorInfo<TextureType>::UNIQUE_TEXTURE_TYPE_STR, materialName->C_Str()) };

		mUniqueTextureName = std::format(L"RawMaterialColorLOD{}_{}_{}", mesh.GetLODScene().GetLODLevel(), Util::General::StringToWString(materialName->C_Str()), MaterialColorInfo<TextureType>::UNIQUE_TEXTURE_TYPE_WSTR);
		mRawMaterialColor = std::move(*materialColor);
		mAlphaValue = MaterialColorInfo<TextureType>::GetMaterialColorAlphaValue(material);
	}

	template <aiTextureType TextureType>
		requires HasMaterialKeyID<TextureType>
	DirectX::ScratchImage MaterialColorModelTextureBuilder<TextureType>::CreateIntermediateScratchTexture() const
	{
		// Assimp stores its material colors as floats in the range [0.0f, 1.0f]. We will convert 
		// these to the R8G8B8A8_UNORM_SRGB format. We choose the _SRGB format with the assumption 
		// that the color is specified as an sRGB value, since this is the color space in which assets 
		// are usually authored in.

		std::array<std::uint8_t, 4> convertedTexelArr{};
		convertedTexelArr[0] = static_cast<std::uint8_t>(mRawMaterialColor.r * 255.0f);
		convertedTexelArr[1] = static_cast<std::uint8_t>(mRawMaterialColor.g * 255.0f);
		convertedTexelArr[2] = static_cast<std::uint8_t>(mRawMaterialColor.b * 255.0f);
		convertedTexelArr[3] = static_cast<std::uint8_t>(mAlphaValue * 255.0f);

		const DirectX::Image texelImage{
			.width = 1,
			.height = 1,
			.format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
			.rowPitch = sizeof(convertedTexelArr),
			.slicePitch = sizeof(convertedTexelArr),
			.pixels = convertedTexelArr.data()
		};

		DirectX::ScratchImage scratchImage{};
		constexpr DXGI_FORMAT INTERMEDIATE_FORMAT = Brawler::GetIntermediateTextureFormat<TextureType>();
		if constexpr (INTERMEDIATE_FORMAT != DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)
		{
			Util::General::CheckHRESULT(DirectX::Convert(
				texelImage,
				Brawler::GetIntermediateTextureFormat<TextureType>(),
				DirectX::TEX_FILTER_FLAGS::TEX_FILTER_DEFAULT,
				DirectX::TEX_THRESHOLD_DEFAULT,
				scratchImage
			));
		}
		else
		{
			Util::General::CheckHRESULT(scratchImage.Initialize1D(
				DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
				1,
				1,
				1
			));

			std::memcpy(scratchImage.GetImage(0, 0, 0)->pixels, convertedTexelArr.data(), sizeof(convertedTexelArr));
		}

		return scratchImage;
	}

	template <aiTextureType TextureType>
		requires HasMaterialKeyID<TextureType>
	std::wstring_view MaterialColorModelTextureBuilder<TextureType>::GetUniqueTextureName() const
	{
		return mUniqueTextureName;
	}
}

#pragma pop_macro("max")