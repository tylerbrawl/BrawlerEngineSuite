module;
#include <vector>
#include <memory>
#include <span>
#include <cassert>
#include <cmath>
#include <assimp/scene.h>

module Brawler.AssimpSceneLoader;
import :AssimpLightLoader;
import Brawler.Math.MathTypes;

namespace Brawler
{
	void AssimpSceneLoader::LoadScene(const aiScene& scene)
	{

	}

	SceneGraph AssimpSceneLoader::ExtractSceneGraph()
	{
		return std::move(mSceneGraph);
	}
}