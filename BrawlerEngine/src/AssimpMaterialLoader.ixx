module;
#include <span>
#include <vector>
#include <assimp/scene.h>
#include <assimp/material.h>

export module Brawler.AssimpSceneLoader:AssimpMaterialLoader;
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
		std::span<const StandardMaterialBuilder> GetMaterialBuilderSpan() const;

	private:
		void CreateSceneTextures(const std::span<const aiMaterial*> materialPtrSpan);

	private:
		std::vector<StandardMaterialBuilder> mMaterialBuilderArr;
	};
}