module;
#include <memory>
#include <assimp/scene.h>

export module Brawler.AssimpSceneLoader:AssimpModelLoader;
import Brawler.SceneNode;

export namespace Brawler
{
	class AssimpModelLoader
	{
	public:
		AssimpModelLoader() = default;

		AssimpModelLoader(const AssimpModelLoader& rhs) = delete;
		AssimpModelLoader& operator=(const AssimpModelLoader& rhs) = delete;

		AssimpModelLoader(AssimpModelLoader&& rhs) noexcept = default;
		AssimpModelLoader& operator=(AssimpModelLoader&& rhs) noexcept = default;

		void LoadModel(const aiScene& scene);
		std::unique_ptr<SceneNode> ExtractModelInstance();

	private:
		std::unique_ptr<SceneNode> mModelInstancePtr;
	};
}