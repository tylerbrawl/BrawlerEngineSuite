module;
#include <vector>
#include <memory>
#include <assimp/mesh.h>
#include <span>
#include <stdexcept>
#include <cassert>

module Brawler.MeshResolverCollection;
import Brawler.MeshTypeID;
import Brawler.JobGroup;

namespace
{
	Brawler::MeshTypeID GetMeshTypeID(const aiMesh& mesh)
	{
		// TODO: Add support for skinned meshes. For now, we assert that the mesh is not
		// skinned and simply create a StaticMeshResolver.

		if (mesh.HasBones()) [[unlikely]]
			throw std::runtime_error{ std::string{ "ERROR: The mesh " } + mesh.mName.C_Str() + " is a skinned mesh. Skinned meshes are currently unsupported." };

		return Brawler::MeshTypeID::STATIC;
	}
}

namespace Brawler
{
	void MeshResolverCollection::CreateMeshResolverForImportedMesh(ImportedMesh&& mesh)
	{
		switch (GetMeshTypeID(mesh.GetMesh()))
		{
		case Brawler::MeshTypeID::STATIC: [[likely]]
		{
			EmplaceMeshResolver<StaticMeshResolver>(std::move(mesh));
			break;
		}

		// TODO: Add support for skinned meshes. For now, we assert that the mesh is not
		// skinned and simply create a StaticMeshResolver.
		case Brawler::MeshTypeID::SKINNED: [[fallthrough]];
		default:
		{
			assert(false && "ERROR: A unique derived MeshResolverBase type was never specified for a given Brawler::MeshTypeID in MeshResolverCollection::CreateMeshResolverForImportedMesh()!");
			std::unreachable();

			break;
		}
		}
	}

	void MeshResolverCollection::Update()
	{
		Brawler::JobGroup meshResolverUpdateGroup{};
		meshResolverUpdateGroup.Reserve(GetMeshResolverCount());

		ForEachMeshResolver([&meshResolverUpdateGroup]<typename T>(T& meshResolver)
		{
			meshResolverUpdateGroup.AddJob([&meshResolver] () { meshResolver.Update(); });
		});

		meshResolverUpdateGroup.ExecuteJobs();
	}

	bool MeshResolverCollection::IsReadyForSerialization() const
	{
		bool readyForSerialization = true;
		ForEachMeshResolver([&readyForSerialization]<typename T>(const T& meshResolver)
		{
			if (!meshResolver.IsReadyForSerialization())
				readyForSerialization = false;
		});

		return readyForSerialization;
	}

	std::size_t MeshResolverCollection::GetMeshResolverCount() const
	{
		std::size_t currSize = 0;
		ForEachMeshResolver([&currSize]<typename T>(const T & meshResolver)
		{
			++currSize;
		});

		return currSize;
	}
}