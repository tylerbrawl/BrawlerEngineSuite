module;
#include <vector>
#include <memory>
#include <unordered_map>
#include <span>
#include <cassert>
#include <filesystem>
#include <format>
#include <assimp/scene.h>

export module Brawler.ModelTextureLUT;
import Brawler.ModelTexture;
import Brawler.LODScene;

export namespace Brawler
{
	template <aiTextureType TextureType>
	class ModelTextureLUT
	{
		struct ModelTextureInfo
		{
			ModelTexture<TextureType> Texture;
			const aiString TextureName;
			LODScene Scene;
		};

	public:
		ModelTextureLUT() = default;

		ModelTextureLUT(const ModelTextureLUT& rhs) = delete;
		ModelTextureLUT& operator=(const ModelTextureLUT& rhs) = delete;

		ModelTextureLUT(ModelTextureLUT&& rhs) noexcept = default;
		ModelTextureLUT& operator=(ModelTextureLUT&& rhs) noexcept = default;

		/// <summary>
		/// Registers all of the textures found within the specified aiScene instance
		/// with the relevant texture type to this ModelTextureLUT instance's map.
		/// 
		/// *WARNING*: This function is *NOT* thread safe! Do *NOT* call this function
		/// while other threads might be reading from or writing to this
		/// ModelTextureLUT instance!
		/// </summary>
		/// <param name="scene">
		/// - The aiScene instance from which textures are to be registered.
		/// </param>
		void AddTexturesFromScene(const LODScene& scene);

	private:
		void AddTexturesFromMaterial(const LODScene& scene, const aiMaterial& material);
		void TryEmplaceModelTexture(const LODScene& scene, const aiString& textureName);

	private:
		std::vector<std::unique_ptr<ModelTextureInfo>> mTextureInfoPtrArr;
		std::unordered_map<std::string, ModelTextureInfo*> mTextureNameMap;
	};
}

// -------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <aiTextureType TextureType>
	void ModelTextureLUT<TextureType>::AddTexturesFromScene(const LODScene& scene)
	{
		const std::span<const aiMaterial*> materialPtrSpan{ const_cast<const aiMaterial**>(scene.GetScene().mMaterials), scene.mNumMaterials};

		for (const auto materialPtr : materialPtrSpan)
			AddTexturesFromMaterial(scene, *materialPtr);
	}

	template <aiTextureType TextureType>
	void ModelTextureLUT<TextureType>::AddTexturesFromMaterial(const LODScene& scene, const aiMaterial& material)
	{
		const std::uint32_t relevantTextureCount = material.GetTextureCount(TextureType);

		for (std::uint32_t i = 0; i < relevantTextureCount; ++i)
		{
			aiString textureName{};
			const aiReturn textureNameResult{ material.GetTexture(TextureType, i, &textureName) };

			assert(textureNameResult == aiReturn::aiReturn_SUCCESS);


		}
	}

	template <aiTextureType TextureType>
	void ModelTextureLUT<TextureType>::TryEmplaceModelTexture(const LODScene& scene, const aiString& textureName)
	{
		// Textures in Assimp may or may not be embedded. If they are not embedded, then their name
		// refers to a file on the actual filesystem. This makes it easy to verify if two texture names
		// refer to the same file.
		//
		// On the other hand, embedded textures are more difficult to correlate. Embedded textures may
		// or may not have a name associated with them. If they have no name, then their name begins
		// with an '*' and is followed by an identifier which is unique for that aiScene instance.
		//
		// It is dangerous to attempt to correlate embedded textures from two different input mesh
		// files. They might share the same texture name, but refer to completely different data.
		// The best way to check if they are equivalent would be to do an SHA-512 hash on their
		// data values, but even this will fail in the case where one texture is just a lower mip level
		// of another existing texture.
		//
		// So, we will make things simple. If the texture is not embedded, then we just compare their
		// file paths to see if they refer to the same file on the system. Otherwise, if the texture
		// is embedded, then we check texture names for equivalence, but only if the textures belong
		// to the same aiScene instance.
		
		const bool isTextureEmbedded = (scene.GetScene().GetEmbeddedTexture(textureName.C_Str()) != nullptr);
		std::string textureNameMapKey{};

		if (isTextureEmbedded) [[unlikely]]
		{
			// To disambiguate between embedded textures with the same name between multiple input mesh
			// files, we will prepend the LOD level to the texture name and use this string as the key
			// to mTextureNameMap.

			textureNameMapKey = std::format("EmbeddedLOD{}_{}", scene.GetLODLevel(), textureName.C_Str());
		}
		else
		{
			// If the texture is not embedded, then it refers to a file on the filesystem. In that case,
			// we need to find it. We first try to search for a file with exactly the given name.
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

			const bool isDirectory = std::filesystem::is_directory(texturePath, errorCode);

			if (errorCode) [[unlikely]]
				throw std::runtime_error{ std::format("ERROR: The attempt to check whether the external texture file \"{}\" was actually a directory or a file failed with the following error: {}", texturePath.string(), errorCode.message()) };

			if (isDirectory) [[unlikely]]
				throw std::runtime_error{ std::format("ERROR: The external texture file \"{}\" was actually a directory!", texturePath.string()) };

			// To ensure that there are no ambiguities, we will always use the canonical path to the texture
			// as the key. This ensures that the file path is always absolute, and that there are no symbolic
			// links in the path.

			texturePath = std::filesystem::canonical(texturePath, errorCode);
			textureNameMapKey = texturePath.string();
		}
	}
}