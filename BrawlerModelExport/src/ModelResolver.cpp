module;
#include <memory>
#include <span>
#include <stdexcept>
#include <vector>
#include <cassert>
#include <assimp/scene.h>

module Brawler.ModelResolver;
import Brawler.MeshTypeID;
import Util.ModelExport;
import Brawler.StaticMeshManager;
import Brawler.MaterialManager;

namespace
{
	/*
	std::unique_ptr<Brawler::I_MeshManager> CreateMeshManager()
	{
		const aiScene& scene{ Util::ModelExport::GetScene() };

		// TODO: Add support for skinned meshes. For now, we assert that the mesh is not
		// skinned and simply create a StaticMeshManager.

		const std::span<const aiMesh*> meshSpan{ const_cast<const aiMesh**>(scene.mMeshes), scene.mNumMeshes };
		for (const auto& meshPtr : meshSpan)
		{
			if (meshPtr->HasBones()) [[unlikely]]
				throw std::runtime_error{ std::string{"ERROR: The mesh "} + meshPtr->mName.C_Str() + " is a skinned mesh. Skinned meshes are not currently supported." };
		}

		return std::make_unique<Brawler::StaticMeshManager>();
	}
	*/

	Brawler::MeshTypeID GetRequiredMeshTypeID(const aiScene& scene)
	{
		// TODO: Add support for skinned meshes. For now, we assert that the mesh is not
		// skinned and simply create a StaticMeshManager.
		
		const std::span<const aiMesh*> meshPtrSpan{ const_cast<const aiMesh**>(scene.mMeshes), scene.mNumMeshes };
		for (const auto meshPtr : meshPtrSpan)
		{
			// NOTE: Remove the [[unlikely]] attribute when skinned meshes are implemented.
			if (meshPtr->HasBones()) [[unlikely]]
				throw std::runtime_error{ std::string{"ERROR: The mesh "} + meshPtr->mName.C_Str() + " is a skinned mesh. Skinned meshes are not currently supported." };
		}

		return Brawler::MeshTypeID::STATIC;
	}
}

namespace Brawler
{
	void ModelResolver::CreateModelResolverComponents()
	{
		CreateMeshManager();
		CreateMaterialManager();
	}

	void ModelResolver::CreateMeshManager()
	{
		const Brawler::MeshTypeID meshTypeID = GetRequiredMeshTypeID(Util::ModelExport::GetScene());

		switch (meshTypeID)
		{
		case Brawler::MeshTypeID::STATIC:
		{
			mComponentPtrArr.push_back(std::make_unique<StaticMeshManager>());
			break;
		}

		// TODO: Add support for skinned meshes. For now, we assert that the mesh is not
		// skinned and simply create a StaticMeshManager.
		case Brawler::MeshTypeID::SKINNED: [[fallthrough]];
		default:
		{
			assert(false);
			std::unreachable();

			break;
		}
		}
	}

	void ModelResolver::CreateMaterialManager()
	{

	}

	/*
	void ModelResolver::ResolveModel()
	{
		// Create a separate CPU job in order to initialize each component of the
		// ModelResolver. These in turn will create their own CPU jobs. In short,
		// we can be sure that all of the cores in the system will receive their
		// fair share of work.
		//
		// For tools such as this model exporter, we really don't need to worry
		// too much about job scheduling or latency. The end goal is to convert
		// the model data, so we can take as long as we need to do that.

		Brawler::JobGroup managerInitializationGroup{};
		managerInitializationGroup.Reserve(2);

		managerInitializationGroup.AddJob([this] ()
		{
			mMaterialManager.BeginInitialization();
		});

		managerInitializationGroup.AddJob([this] ()
		{
			mMeshManager = CreateMeshManager();
			mMeshManager->BeginInitialization();
		});

		managerInitializationGroup.ExecuteJobs();
	}

	void ModelResolver::SerializeModel() const
	{

	}
	*/
}