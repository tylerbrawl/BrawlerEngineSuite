module;
#include <vector>
#include <memory>
#include <span>
#include <cassert>
#include <assimp/scene.h>

module Brawler.StaticMeshManager;
import Util.ModelExport;
import Brawler.JobSystem;

namespace Brawler
{
	StaticMeshManager::StaticMeshManager() :
		mMeshArr(),
		mActiveSerializationsCounter(0)
	{
		CreateSerializationJobs();
	}

	void StaticMeshManager::Update()
	{}

	bool StaticMeshManager::AreMAPsSerialized() const
	{
		// If this function is called, then we can expect the thread which called this function
		// to soon access this StaticMeshManager's StaticMesh pointers. Thus, we use a
		// read-acquire memory ordering here to synchronize with the writes done in the serialization
		// jobs.

		return (mActiveSerializationsCounter.load(std::memory_order::acquire) == 0);
	}

	void StaticMeshManager::CreateSerializationJobs()
	{
		const aiScene& scene{ Util::ModelExport::GetScene() };

		const std::size_t meshCount = static_cast<std::size_t>(scene.mNumMeshes);
		mMeshArr.resize(meshCount);

		mActiveSerializationsCounter.store(scene.mNumMeshes, std::memory_order::relaxed);

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
			meshInitializationJobGroup.AddJob([this, &staticMeshPtr, meshPtr = meshSpan[i]]()
			{
				staticMeshPtr = std::make_unique<StaticMesh>(*meshPtr);

				// Once the job is finished, decrement the active serialization count. We use a write-release
				// memory ordering here to ensure that the thread which calls StaticMeshManager::Update() again
				// gets the correct value.
				mActiveSerializationsCounter.fetch_sub(1, std::memory_order::release);
			});
		}

		meshInitializationJobGroup.ExecuteJobsAsync();
		mSerializationJobsCreated = true;
	}
}