module;
#include <string>
#include <stdexcept>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "DxDef.h"

module Brawler.SceneLoader;
import Util.General;

namespace Brawler
{
	SceneLoader::SceneLoader(const std::wstring_view inputModelFilePath) :
		mAssimpImporter(),
		mAiScene(nullptr),
		mModelResolver()
	{
		mAssimpImporter.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);

		mAiScene = mAssimpImporter.ReadFile(
			Util::General::WStringToString(inputModelFilePath),
			aiProcess_ConvertToLeftHanded			|
			aiProcess_CalcTangentSpace				|
			aiProcess_RemoveRedundantMaterials		|
			aiProcess_JoinIdenticalVertices			|
			aiProcess_Triangulate					|
			aiProcess_SortByPType					|
			aiProcess_OptimizeMeshes				|
			aiProcess_OptimizeGraph
		);

		if (mAiScene == nullptr) [[unlikely]]
			throw std::runtime_error{ "ERROR: The provided model file could not be imported!" };
	}

	void SceneLoader::ProcessScene()
	{
		mModelResolver.ResolveModel();
		mModelResolver.SerializeModel();
	}

	const aiScene& SceneLoader::GetScene() const
	{
		return *mAiScene;
	}
}