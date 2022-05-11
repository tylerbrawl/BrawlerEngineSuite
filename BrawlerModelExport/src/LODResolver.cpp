module;
#include <filesystem>
#include <stdexcept>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <cassert>
#include <algorithm>
#include <span>

module Brawler.LODResolver;

namespace Brawler
{
	LODResolver::LODResolver(const std::uint32_t lodLevel) :
		mImporter(),
		mAIScenePtr(nullptr),
		mMeshResolverCollection(),
		mLODLevel(lodLevel)
	{}

	void LODResolver::ImportScene(const std::filesystem::path& fbxFile)
	{
		CreateAIScene(fbxFile);
		CreateMeshResolvers();
	}

	void LODResolver::Update()
	{
		mMeshResolverCollection.Update();
	}

	bool LODResolver::IsReadyForSerialization() const
	{
		return mMeshResolverCollection.IsReadyForSerialization();
	}

	const aiScene& LODResolver::GetScene() const
	{
		assert(mAIScenePtr != nullptr);
		return *mAIScenePtr;
	}

	std::uint32_t LODResolver::GetLODLevel() const
	{
		return mLODLevel;
	}

	void LODResolver::CreateAIScene(const std::filesystem::path& fbxFile)
	{
		// Check that the provided file is an FBX file. Although Assimp supports files of arbitrary
		// types, there are certain properties of FBX files which we assume throughout the import
		// process. For instance, FBX's unit system is 1 unit = 1 centimeter, but the Brawler Engine uses
		// 1 unit = 1 meter.
		//
		// We only check for file extension, rather than for the magic header of the file, since this
		// program is just a tool for our use.
		{
			const std::filesystem::path fileExtension{ fbxFile.extension() };
			
			std::string fileExtStr{ fileExtension.string() };
			std::ranges::transform(fileExtStr, fileExtStr.begin(), [] (const char c) { return std::tolower(c); });

			if (fileExtStr != ".fbx") [[unlikely]]
				throw std::runtime_error{ std::string{"ERROR: The model file "} + fbxFile.string() + " is not a valid .FBX file!" };
		}
		
		mImporter.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType::aiPrimitiveType_LINE | aiPrimitiveType::aiPrimitiveType_POINT);

		mAIScenePtr = mImporter.ReadFile(
			fbxFile.string(),
			aiProcess_ConvertToLeftHanded |
			aiPostProcessSteps::aiProcess_CalcTangentSpace |
			aiPostProcessSteps::aiProcess_RemoveRedundantMaterials |
			aiPostProcessSteps::aiProcess_JoinIdenticalVertices |
			aiPostProcessSteps::aiProcess_Triangulate |
			aiPostProcessSteps::aiProcess_SortByPType |
			aiPostProcessSteps::aiProcess_OptimizeMeshes |
			aiPostProcessSteps::aiProcess_OptimizeGraph
		);

		if (mAiScenePtr == nullptr) [[unlikely]]
			throw std::runtime_error{ std::string{ "ERROR: The model file " } + fbxFile.string() + " could not be imported!" };
	}

	void LODResolver::CreateMeshResolvers()
	{
		const std::span<const aiMesh*> meshSpan{ const_cast<const aiMesh**>(mAIScenePtr->mMeshes), mAIScenePtr->mNumMeshes };

		for (const auto meshPtr : meshSpan)
			mMeshResolverCollection.CreateMeshResolverForAIMesh(*meshPtr);
	}
}