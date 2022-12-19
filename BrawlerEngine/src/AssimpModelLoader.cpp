module;
#include <memory>
#include <queue>
#include <span>
#include <cassert>
#include <vector>
#include <ranges>
#include <string_view>
#include <assimp/scene.h>

module Brawler.AssimpSceneLoader;
import :AssimpMaterialLoader;
import :AssimpMeshBuilder;
import :AssimpLoadingUtil;
import Brawler.ModelInstanceComponent;
import Brawler.JobSystem;
import Brawler.StandardMaterialDefinition;
import Brawler.MaterialDefinitionGraph;
import Brawler.Mesh;
import Brawler.Model;
import Brawler.FilePathHash;
import Brawler.TransformComponent;
import Brawler.ModelInstanceComponent;

namespace
{
	std::size_t GetNodesContainingMeshDataCount(const aiScene& scene)
	{
		std::size_t meshNodeCount = 0;

		// Do a breadth-first search (BFS) to count the number of nodes which contain
		// mesh data.
		std::queue<const aiNode*> remainingNodePtrQueue{};
		remainingNodePtrQueue.push(scene.mRootNode);

		while (!remainingNodePtrQueue.empty())
		{
			const aiNode* const currNodePtr = remainingNodePtrQueue.front();
			remainingNodePtrQueue.pop();

			if (currNodePtr->mNumMeshes > 0)
				++meshNodeCount;

			const std::span<const aiNode*> childNodePtrSpan{ currNodePtr->mChildren, currNodePtr->mNumChildren };

			for (const auto childNodePtr : childNodePtrSpan)
				remainingNodePtrQueue.push(childNodePtr);
		}

		return meshNodeCount;
	}
}

namespace Brawler
{
	void AssimpModelLoader::LoadModel(const AssimpSceneLoadParams& params)
	{
		// We use aiPostProcessSteps::aiProcess_OptimizeGraph when importing scenes with Assimp,
		// so there should only be one aiNode instance with actual mesh data.
		assert(GetNodesContainingMeshDataCount(params.AssimpScene) == 1 && "ERROR: An attempt was made to load a scene using Assimp, but the resulting aiScene instance did not have exactly one node with aiMesh references! (Did you remember to use aiPostProcessSteps::aiProcess_OptimizeGraph?)");

		// There are two things we need to load models:
		//
		//   1. Mesh Data (Basically, this is anything related to the geometry of the model.)
		//   2. Material Data
		//
		// These two components can be loaded concurrently, and when they are finished, the resulting
		// MaterialDefinitionHandle instances can be associated with the corresponding Mesh instances
		// to complete the model load process.
		AssimpMaterialLoader materialLoader{};
		const aiNode* const meshDataNodePtr = [&scene = params.AssimpScene] ()
		{
			std::queue<const aiNode*> remainingNodePtrQueue{};
			remainingNodePtrQueue.push(scene.mRootNode);

			while (!remainingNodePtrQueue.empty())
			{
				const aiNode* const currNodePtr = remainingNodePtrQueue.front();
				remainingNodePtrQueue.pop();

				if (currNodePtr->mNumMeshes > 0)
					return currNodePtr;

				const std::span<const aiNode*> childNodePtrSpan{ currNodePtr->mChildren, currNodePtr->mNumChildren };

				for (const auto childNodePtr : childNodePtrSpan)
					remainingNodePtrQueue.push(childNodePtr);
			}

			return nullptr;
		}();

		assert(meshDataNodePtr != nullptr);

		const std::span<const std::uint32_t> meshIndexSpan{ meshDataNodePtr->mMeshes, meshDataNodePtr->mNumMeshes };
		const auto meshRange{ meshIndexSpan | std::views::transform([&scene = params.AssimpScene] (const std::uint32_t meshIndex) -> const aiMesh&
		{
			assert(meshIndex < scene.mNumMeshes);

			const aiMesh* const currMeshPtr = scene.mMeshes[meshIndex];
			assert(currMeshPtr != nullptr);

			return *currMeshPtr;
		}) };

		std::vector<AssimpMeshBuilder> meshBuilderArr{};
		meshBuilderArr.resize(meshIndexSpan.size());

		// Concurrently load the material and mesh data.
		Brawler::JobGroup modelLoadGroup{};
		modelLoadGroup.Reserve(meshBuilderArr.size() + 1);

		modelLoadGroup.AddJob([&materialLoader, &scene = params.AssimpScene] ()
		{
			materialLoader.LoadMaterials(scene);
		});

		for (auto& [currMeshBuilder, currMesh] : std::views::zip(meshBuilderArr, meshRange))
		{
			modelLoadGroup.AddJob([&currMeshBuilder, &currMesh] ()
			{
				currMeshBuilder.InitializeMeshData(currMesh);
			});
		}

		modelLoadGroup.ExecuteJobs();

		// Associate meshes with their corresponding MaterialDefinitionHandle instances.
		// Then, create the Mesh instances which will comprise the created Model instance.
		const std::span<const StandardMaterialBuilder> materialBuilderSpan{ materialLoader.GetMaterialBuilderSpan() };

		std::vector<Mesh> createdMeshArr{};
		createdMeshArr.reserve(meshBuilderArr.size());

		for (auto& [currMeshBuilder, currMesh] : std::views::zip(meshBuilderArr, meshRange))
		{
			assert(currMesh.mMaterialIndex < materialBuilderSpan.size());

			const StandardMaterialBuilder& currMaterialBuilder{ materialBuilderSpan[currMesh.mMaterialIndex] };
			currMeshBuilder.SetMaterialDefinitionHandle(MaterialDefinitionGraph::GetInstance().CreateMaterialDefinition<StandardMaterialDefinition>(currMaterialBuilder));

			createdMeshArr.push_back(currMeshBuilder.CreateMesh());
		}

		// Create the Model instance and register it with the ModelDatabase.
		ModelHandle hModel{};

		{
			Model createdModel{ std::span<Mesh>{ createdMeshArr } };

			const FilePathHash scenePathHash{ Brawler::CONSTRUCT_FILE_PATH_HASH_AT_RUNTIME, std::wstring_view{ params.SceneFilePath.c_str() } };
			hModel = ModelDatabase::GetInstance().AddSharedModel(scenePathHash, std::move(createdModel));
		}

		// Finally, we can create the model instance as a generic SceneNode with a TransformComponent
		// and ModelInstanceComponent attached to it.
		mModelInstancePtr = std::make_unique<SceneNode>();

		mModelInstancePtr->CreateComponent<TransformComponent>();
		Util::AssimpLoading::TransformSceneNode(*mModelInstancePtr, *meshDataNodePtr);

		mModelInstancePtr->CreateComponent<ModelInstanceComponent>(std::move(hModel));
	}

	std::unique_ptr<SceneNode> AssimpModelLoader::ExtractModelInstance()
	{
		return std::move(mModelInstancePtr);
	}
}