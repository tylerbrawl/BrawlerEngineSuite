module;
#include <span>
#include <vector>
#include <assimp/scene.h>
#include <assimp/material.h>

export module Brawler.AssimpSceneLoader:AssimpMaterialLoader;
import Brawler.MaterialDefinitionHandle;
import Brawler.StandardMaterialDefinition;

export namespace Brawler 
{
	class AssimpMaterialLoader
	{
	public:
		AssimpMaterialLoader() = default;

		AssimpMaterialLoader(const AssimpMaterialLoader& rhs) = default;
		AssimpMaterialLoader& operator=(const AssimpMaterialLoader& rhs) = default;

		AssimpMaterialLoader(AssimpMaterialLoader&& rhs) noexcept = default;
		AssimpMaterialLoader& operator=(AssimpMaterialLoader&& rhs) noexcept = default;

		void LoadMaterials(const aiScene& scene);

		std::span<MaterialDefinitionHandle> GetMaterialDefinitionHandleSpan();
		std::span<const MaterialDefinitionHandle> GetMaterialDefinitionHandleSpan() const;

	private:
		void CreateSceneTextures(const std::span<const aiMaterial*> materialPtrSpan);
		void CreateMaterialDefinitions();

	private:
		std::vector<MaterialDefinitionHandle> mHMaterialDefinitionArr;
		std::vector<StandardMaterialBuilder> mMaterialBuilderArr;
	};
}