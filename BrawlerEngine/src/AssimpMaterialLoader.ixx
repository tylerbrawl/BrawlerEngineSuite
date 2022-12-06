module;
#include <span>
#include <vector>
#include <assimp/scene.h>

export module Brawler.AssimpMaterialLoader;
import Brawler.MaterialDefinitionHandle;

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
		std::vector<MaterialDefinitionHandle> mHMaterialDefinitionArr;
	};
}