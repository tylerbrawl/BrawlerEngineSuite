module;
#include <assimp/scene.h>

export module Brawler.AssimpSceneLoader;
import Brawler.SceneGraph;
import Brawler.SceneNode;

export namespace Brawler
{
	class AssimpSceneLoader
	{
	public:
		AssimpSceneLoader() = default;

		AssimpSceneLoader(const AssimpSceneLoader& rhs) = delete;
		AssimpSceneLoader& operator=(const AssimpSceneLoader& rhs) = delete;

		AssimpSceneLoader(AssimpSceneLoader&& rhs) noexcept = default;
		AssimpSceneLoader& operator=(AssimpSceneLoader&& rhs) noexcept = default;

		void LoadScene(const aiScene& scene);

		SceneGraph ExtractSceneGraph();

	private:
		SceneGraph mSceneGraph;
	};
}