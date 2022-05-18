module;
#include <optional>
#include <filesystem>
#include <string_view>
#include <format>
#include <stdexcept>
#include <assimp/material.h>
#include <DirectXTex.h>

export module Brawler.I_ModelTextureBuilder;
import Brawler.OptionalRef;

export namespace Brawler
{
	template <aiTextureType TextureType>
	class I_ModelTextureBuilder
	{
	protected:
		I_ModelTextureBuilder() = default;

	public:
		virtual ~I_ModelTextureBuilder() = default;

		I_ModelTextureBuilder(const I_ModelTextureBuilder& rhs) = delete;
		I_ModelTextureBuilder& operator=(const I_ModelTextureBuilder& rhs) = delete;

		I_ModelTextureBuilder(I_ModelTextureBuilder&& rhs) noexcept = default;
		I_ModelTextureBuilder& operator=(I_ModelTextureBuilder&& rhs) noexcept = default;

		virtual DirectX::ScratchImage CreateIntermediateScratchTexture() const = 0;
		virtual std::wstring_view GetUniqueTextureName() const = 0;

		bool IsDuplicateTexture(const I_ModelTextureBuilder& otherBuilder) const;
		virtual OptionalRef<const std::filesystem::path> GetImportedTextureFilePath() const;
	};
}

// ------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <aiTextureType TextureType>
	bool I_ModelTextureBuilder<TextureType>::IsDuplicateTexture(const I_ModelTextureBuilder& otherBuilder) const
	{
		// It's only really safe to check if two I_ModelTextureBuilder instances are referring to the
		// same texture if both of them are referring to an external texture file. However, this is
		// likely to be the common case.

		const OptionalRef<const std::filesystem::path> thisTextureFilePath{ GetImportedTextureFilePath() };
		const OptionalRef<const std::filesystem::path> otherBuilderTextureFilePath{ otherBuilder.GetImportedTextureFilePath() };

		if (!thisTextureFilePath.HasValue() || !otherBuilderTextureFilePath.HasValue()) [[unlikely]]
			return false;

		std::error_code errorCode{};
		const bool isDuplicate = std::filesystem::equivalent(*thisTextureFilePath, *otherBuilderTextureFilePath, errorCode);

		if (errorCode) [[unlikely]]
			throw std::runtime_error{ std::format("ERROR: The attempt to check if two I_ModelTextureBuilder instances referring to external texture files were referring to the same file failed with the following error: {}", errorCode.message()) };

		return isDuplicate;
	}

	template <aiTextureType TextureType>
	OptionalRef<const std::filesystem::path> I_ModelTextureBuilder<TextureType>::GetImportedTextureFilePath() const
	{
		return OptionalRef<const std::filesystem::path>{};
	}
}