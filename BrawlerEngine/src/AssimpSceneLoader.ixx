module;
#include <memory>
#include <assimp/scene.h>

export module Brawler.AssimpSceneLoader;
export import :AssimpSceneLoadParams;
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

		void LoadScene(const AssimpSceneLoadParams& params);
		std::unique_ptr<SceneGraph> ExtractSceneGraph();

	private:
		std::unique_ptr<SceneGraph> mSceneGraphPtr;
	};
}