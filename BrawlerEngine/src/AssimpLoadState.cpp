module;
#include <filesystem>
#include <stdexcept>
#include <format>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

module Brawler.AssimpLoadState;

namespace Brawler 
{
	AssimpLoadState::AssimpLoadState(std::filesystem::path modelFilePath) :
		mImporter(),
		mLoadedWorld(),
		mScenePtr(),
		mModelFilePath(std::move(modelFilePath))
	{}

	void AssimpLoadState::Update(const float)
	{
		// First, import the model using Assimp. This must be done on a single thread,
		// unfortunately.
		ImportModelFile();


	}

	void AssimpLoadState::ImportModelFile()
	{
		// Remove all lines and points from the model.
		mImporter.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType::aiPrimitiveType_LINE | aiPrimitiveType::aiPrimitiveType_POINT);

		// FBX files use 1 unit = 1 cm, but the Brawler Engine uses 1 unit = 1 meter. So,
		// we scale down the imported model.
		mImporter.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, 0.01f);

		static constexpr aiPostProcessSteps ASSIMP_POST_PROCESS_STEPS = (
			aiProcessPreset_TargetRealtime_MaxQuality |
			aiProcess_ConvertToLeftHanded |
			aiPostProcessSteps::aiProcess_TransformUVCoords |
			aiPostProcessSteps::aiProcess_GlobalScale |
			aiPostProcessSteps::aiProcess_OptimizeGraph
		);

		const std::string modelFilePathStr{ mModelFilePath.string() };
		mScenePtr = mImporter.ReadFile(modelFilePathStr, ASSIMP_POST_PROCESS_STEPS);

		if (mScenePtr == nullptr) [[unlikely]]
			throw std::runtime_error{ std::format(R"(ERROR: The model file "{}" could not be imported by Assimp!)", modelFilePathStr) };
	}
}