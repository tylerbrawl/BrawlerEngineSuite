module;
#include <span>
#include <filesystem>
#include <assimp/scene.h>
#include <cassert>
#include <optional>
#include <DirectXTex.h>
#include "DxDef.h"

export module Util.ModelTexture;
import Brawler.TextureTypeMap;
import Brawler.LaunchParams;
import Util.ModelExport;
import Util.Win32;
import Brawler.Win32.ConsoleFormat;
import Util.General;
import Brawler.FilePathHash;
import Brawler.LODScene;
import Brawler.AssimpMaterialKey;
import Brawler.I_ModelTextureBuilder;
import Brawler.ExternalTextureModelTextureBuilder;
import Brawler.EmbeddedTextureModelTextureBuilder;
import Brawler.ImportedMesh;

export namespace Util
{
	namespace ModelTexture
	{
		constexpr std::string_view EMBEDDED_TEXTURE_NAME_FORMAT_STR = "EmbeddedLOD{}_{}";

		struct TextureWriteInfo
		{
			/// <summary>
			/// The full file path, including the extension, of the .btex file which is to be
			/// written.
			/// </summary>
			std::filesystem::path OutputDirectory;

			/// <summary>
			/// The DirectX12 resource description which describes this texture.
			/// </summary>
			Brawler::D3D12_RESOURCE_DESC ResourceDescription;

			/// <summary>
			/// A DirectX::Blob instance containing the texture data saved as a DDS file. This
			/// can/should be created with DirectX::SaveToDDSMemory().
			/// </summary>
			DirectX::Blob DDSBlob;
		};

		/// <summary>
		/// Writes the texture data specified by writeInfo to the filesystem. See the documentation
		/// of Util::ModelTexture::TextureWriteInfo for more information.
		/// </summary>
		/// <param name="writeInfo">
		/// - A TextureWriteInfo instance which includes all of the necessary information for
		///   writing out the texture file.
		/// </param>
		void WriteTextureToFile(const TextureWriteInfo& writeInfo);

		template <aiTextureType TextureType>
		std::optional<std::unique_ptr<Brawler::I_ModelTextureBuilder<TextureType>>> CreateModelTextureBuilderForExistingTexture(const Brawler::ImportedMesh& mesh, const std::uint32_t textureIndex = 0);
	}
}

// --------------------------------------------------------------------------------------------

namespace Util
{
	namespace ModelTexture
	{
		template <aiTextureType TextureType>
		std::optional<std::unique_ptr<Brawler::I_ModelTextureBuilder<TextureType>>> CreateModelTextureBuilderForExistingTexture(const Brawler::ImportedMesh& mesh, const std::uint32_t textureIndex)
		{
			// Check if this is an embedded texture. If so, then we need to return an EmbeddedTextureModelTextureBuilder.
			// Otherwise, we need to return an ExternalTextureModelTextureBuilder.
			//
			// We expect most textures to be external.
			aiString textureName{};
			const aiReturn getTextureResult{ mesh.GetMeshMaterial().GetTexture(TextureType, textureIndex, &textureName) };

			if (getTextureResult != aiReturn::aiReturn_SUCCESS) [[unlikely]]
				return std::optional<std::unique_ptr<Brawler::I_ModelTextureBuilder<TextureType>>>{};

			const Brawler::LODScene scene{ mesh.GetLODScene() };
			
			if (scene.GetScene().GetEmbeddedTexture(textureName.C_Str()) != nullptr) [[unlikely]]
				return std::make_unique<Brawler::EmbeddedTextureModelTextureBuilder<TextureType>>(scene, textureName);

			return std::make_unique<Brawler::ExternalTextureModelTextureBuilder<TextureType>>(scene, textureName);
		}
	}
}