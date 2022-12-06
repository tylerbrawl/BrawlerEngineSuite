module;
#include <memory>

export module Brawler.World;
import Brawler.SceneGraph;

export namespace Brawler
{
	class World
	{
	public:
		World() = default;
		explicit World(std::unique_ptr<SceneGraph>&& sceneGraphPtr);

		World(const World& rhs) = delete;
		World& operator=(const World& rhs) = delete;

		World(World&& rhs) noexcept = default;
		World& operator=(World&& rhs) noexcept = default;

	private:
		std::unique_ptr<SceneGraph> mSceneGraphPtr;
	};
}