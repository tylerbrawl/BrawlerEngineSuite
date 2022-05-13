module;
#include <filesystem>
#include <cassert>
#include <assimp/scene.h>

module Brawler.LODScene;
import Util.ModelExport;
import Brawler.AppParams;

namespace Brawler
{
	LODScene::LODScene(const aiScene& scene, const std::uint32_t lodLevel) :
		mAIScenePtr(&scene),
		mLODLevel(lodLevel)
	{}

	const aiScene& LODScene::GetScene() const
	{
		assert(mAIScenePtr != nullptr);
		return *mAIScenePtr;
	}

	std::uint32_t LODScene::GetLODLevel() const
	{
		return mLODLevel;
	}

	const std::filesystem::path& LODScene::GetInputMeshFilePath() const
	{
		return Util::ModelExport::GetLaunchParameters().GetLODFilePath(mLODLevel);
	}
}