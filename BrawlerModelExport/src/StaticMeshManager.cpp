module;
#include <vector>
#include <memory>
#include <span>
#include <cassert>
#include <assimp/scene.h>

module Brawler.StaticMeshManager;
import Util.General;
import Brawler.JobSystem;

namespace Brawler
{
	void StaticMeshManager::BeginInitialization()
	{
		const aiScene& scene{ Util::General::GetScene() };

		const std::size_t meshCount = static_cast<std::size_t>(scene.mNumMeshes);
		mMeshArr.resize(meshCount);

		Brawler::JobGroup meshInitializationJobGroup{};
		meshInitializationJobGroup.Reserve(meshCount);

		const std::span<const aiMesh*> meshSpan{ const_cast<const aiMesh**>(scene.mMeshes), meshCount };
		for (std::size_t i = 0; i < meshCount; ++i)
		{
			// The StaticMeshManager assumes that all of the aiMesh instances in a scene are static and
			// not skinned. We assert that this is the case.
			assert(!(meshSpan[i]->HasBones()) && "ERROR: An aiMesh was found in the loaded scene which has bones, but it was sent to a StaticMeshManager!");

			// Create a CPU job for every mesh which is to be initialized. The actual process of initializing
			// the meshes can take some time, especially since mesh vertices need to be packed appropriately.
			std::unique_ptr<StaticMesh>& staticMeshPtr{ mMeshArr[i] };
			meshInitializationJobGroup.AddJob([&staticMeshPtr, meshPtr = meshSpan[i]]()
			{
				staticMeshPtr = std::make_unique<StaticMesh>(*meshPtr);
			});
		}

		meshInitializationJobGroup.ExecuteJobs();
	}

	MeshTypeID StaticMeshManager::GetMeshTypeID() const
	{
		return MeshTypeID::STATIC;
	}
}