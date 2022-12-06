module;
#include <memory>

module Brawler.World;

namespace Brawler
{
	World::World(std::unique_ptr<SceneGraph>&& sceneGraphPtr) :
		mSceneGraphPtr(std::move(sceneGraphPtr))
	{}
}