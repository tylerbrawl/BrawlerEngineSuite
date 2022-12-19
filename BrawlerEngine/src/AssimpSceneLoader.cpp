module;
#include <vector>
#include <memory>
#include <span>
#include <cassert>
#include <cmath>
#include <assimp/scene.h>

module Brawler.AssimpSceneLoader;
import :AssimpLightLoader;
import :AssimpModelLoader;
import Brawler.Math.MathTypes;
import Brawler.JobSystem;

namespace Brawler
{
	void AssimpSceneLoader::LoadScene(const AssimpSceneLoadParams& params)
	{
		// Assimp imports both lights and model data within a scene. We can create SceneNode instances
		// for these two types concurrently.
		AssimpModelLoader modelLoader{};
		AssimpLightLoader lightLoader{};

		Brawler::JobGroup sceneLoadGroup{};
		sceneLoadGroup.Reserve(2);

		sceneLoadGroup.AddJob([&modelLoader, &params] ()
		{
			modelLoader.LoadModel(params);
		});

		sceneLoadGroup.AddJob([&lightLoader, &scene = params.AssimpScene] ()
		{
			lightLoader.LoadLights(scene);
		});

		sceneLoadGroup.ExecuteJobs();

		mSceneGraphPtr = std::make_unique<SceneGraph>();

		// Add the SceneNode instances to the SceneGraph.
		mSceneGraphPtr->AddRootLevelSceneNode(modelLoader.ExtractModelInstance());

		for (auto&& lightSceneNode : lightLoader.GetLightSceneNodeSpan())
			mSceneGraphPtr->AddRootLevelSceneNode(std::move(lightSceneNode));
	}

	std::unique_ptr<SceneGraph> AssimpSceneLoader::ExtractSceneGraph()
	{
		return std::move(mSceneGraphPtr);
	}
}