module;
#include <optional>
#include <filesystem>
#include <cassert>
#include <string>
#include <assimp/scene.h>
#include <DirectXTex.h>

export module Brawler.ExternalTextureModelTextureBuilder;
import Brawler.I_ModelTextureBuilder;
import Brawler.LODScene;
import Util.General;
import Brawler.TextureTypeMap;
import Brawler.FilePathHash;

export namespace Brawler
{
	template <aiTextureType TextureType>
	class ExternalTextureModelTextureBuilder final : public I_ModelTextureBuilder<TextureType>
	{
	public:
		ExternalTextureModelTextureBuilder(const Brawler::LODScene& scene, const aiString& textureName);

		ExternalTextureModelTextureBuilder(const ExternalTextureModelTextureBuilder& rhs) = delete;
		ExternalTextureModelTextureBuilder& operator=(const ExternalTextureModelTextureBuilder& rhs) = delete;

		ExternalTextureModelTextureBuilder(ExternalTextureModelTextureBuilder&& rhs) noexcept = default;
		ExternalTextureModelTextureBuilder& operator=(ExternalTextureModelTextureBuilder&& rhs) noexcept = default;

		DirectX::ScratchImage CreateIntermediateScratchTexture() const override;
		std::wstring_view GetUniqueTextureName() const override;

		OptionalRef<const std::filesystem::path> GetImportedTextureFilePath() const override;

	private:
		void InitializeTextureFilePath(const Brawler::LODScene& scene, const aiString& textureName);

	private:
		std::filesystem::path mTextureFilePath;
		std::wstring mUniqueTextureName;
	};
}

// ----------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <aiTextureType TextureType>
	ExternalTextureModelTextureBuilder<TextureType>::ExternalTextureModelTextureBuilder(const Brawler::LODScene& scene, const aiString& textureName) :
		I_ModelTextureBuilder<TextureType>(),
		mTextureFilePath(),
		mUniqueTextureName()
	{
		InitializeTextureFilePath(scene, textureName);
	}

	template <aiTextureType TextureType>
	DirectX::ScratchImage ExternalTextureModelTextureBuilder<TextureType>::CreateIntermediateScratchTexture() const
	{
		// Let DirectXTex load the file, assuming that it is in a WIC-compatible format.
		DirectX::ScratchImage wicImage{};

		Util::General::CheckHRESULT(DirectX::LoadFromWICFile(
			mTextureFilePath.c_str(),
			DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
			nullptr,
			wicImage
		));

		DirectX::ScratchImage scratchImage{};
		Util::General::CheckHRESULT(DirectX::Convert(
			wicImage.GetImages(),
			wicImage.GetImageCount(),
			wicImage.GetMetadata(),
			Brawler::GetIntermediateTextureFormat<TextureType>(),
			DirectX::TEX_FILTER_FLAGS::TEX_FILTER_DEFAULT,
			DirectX::TEX_THRESHOLD_DEFAULT,
			scratchImage
		));

		return scratchImage;
	}

	template <aiTextureType TextureType>
	std::wstring_view ExternalTextureModelTextureBuilder<TextureType>::GetUniqueTextureName() const
	{
		return mUniqueTextureName;
	}

	template <aiTextureType TextureType>
	OptionalRef<const std::filesystem::path> ExternalTextureModelTextureBuilder<TextureType>::GetImportedTextureFilePath() const
	{
		return OptionalRef<const std::filesystem::path>{ mTextureFilePath };
	}

	template <aiTextureType TextureType>
	void ExternalTextureModelTextureBuilder<TextureType>::InitializeTextureFilePath(const Brawler::LODScene& scene, const aiString& textureName)
	{
		assert(scene.GetScene().GetEmbeddedTexture(textureName.C_Str()) == nullptr && "ERROR: An attempt was made to use an ExternalTextureModelTextureBuilder to import an embedded texture, rather than an external one!");

		// We first try to search for a file with exactly the given name.
		std::filesystem::path texturePath{ textureName.C_Str() };
		std::error_code errorCode{};

		bool doesTexturePathExist = std::filesystem::exists(texturePath, errorCode);

		if (errorCode) [[unlikely]]
			throw std::runtime_error{ std::format("ERROR: The attempt to check whether or not the external texture file \"{}\" exists resulted in the following error: {}", texturePath.string(), errorCode.message()) };

		if (!doesTexturePathExist)
		{
			// If we could not find the texture like that, then it is possible that the texture
			// path was described relative to the input mesh file itself.
			const std::filesystem::path inputMeshFileDirectory{ scene.GetInputMeshFilePath().parent_path() };
			texturePath = (inputMeshFileDirectory / texturePath.filename());

			doesTexturePathExist = std::filesystem::exists(texturePath, errorCode);

			if (errorCode) [[unlikely]]
				throw std::runtime_error{ std::format("ERROR: The attempt to check whether or not the external texture file \"{}\" exists resulted in the following error: {}", texturePath.string(), errorCode.message()) };

			if (!doesTexturePathExist) [[unlikely]]
				throw std::runtime_error{ std::format("ERROR: The external texture file \"{}\" could not be found!", texturePath.string()) };
		}

		// Make sure that the texture which we just found is actually a file.
		const bool isDirectory = std::filesystem::is_directory(texturePath, errorCode);

		if (errorCode) [[unlikely]]
			throw std::runtime_error{ std::format("ERROR: The attempt to check whether the external texture file \"{}\" was actually a directory or a file failed with the following error: {}", texturePath.string(), errorCode.message()) };

		if (isDirectory) [[unlikely]]
			throw std::runtime_error{ std::format("ERROR: The external texture file \"{}\" was actually a directory!", texturePath.string()) };

		mTextureFilePath = std::move(texturePath);

		const std::filesystem::path canonicalPath{ std::filesystem::canonical(mTextureFilePath, errorCode) };

		if (errorCode) [[unlikely]]
			throw std::runtime_error{ std::format("ERROR: The attempt to get the canonical file path of the external texture file \"{}\" failed with the following error: {}", mTextureFilePath.string(), errorCode.message()) };

		const FilePathHash canonicalPathHash{ canonicalPath.c_str() };
		mUniqueTextureName = std::format(L"{}_{}", mTextureFilePath.stem().wstring(), Util::General::StringToWString(canonicalPathHash.GetHashString()));
	}
}