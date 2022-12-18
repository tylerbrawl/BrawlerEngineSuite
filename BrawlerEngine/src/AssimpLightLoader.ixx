module;
#include <memory>
#include <vector>
#include <span>
#include <assimp/scene.h>

export module Brawler.AssimpSceneLoader:AssimpLightLoader;
import Brawler.SceneNode;

export namespace Brawler
{
	class AssimpLightLoader
	{
	public:
		AssimpLightLoader() = default;

		AssimpLightLoader(const AssimpLightLoader& rhs) = delete;
		AssimpLightLoader& operator=(const AssimpLightLoader& rhs) = delete;

		AssimpLightLoader(AssimpLightLoader&& rhs) noexcept = default;
		AssimpLightLoader& operator=(AssimpLightLoader&& rhs) noexcept = default;

		void LoadLights(const aiScene& scene);

		std::span<std::unique_ptr<SceneNode>> GetLightSceneNodeSpan();
		std::span<const std::unique_ptr<SceneNode>> GetLightSceneNodeSpan() const;

	private:
		std::vector<std::unique_ptr<SceneNode>> mLightSceneNodePtrArr;
	};
}