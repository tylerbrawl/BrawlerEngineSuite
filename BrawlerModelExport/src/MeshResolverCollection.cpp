module;
#include <vector>
#include <memory>
#include <assimp/mesh.h>
#include <span>
#include <stdexcept>
#include <cassert>

module Brawler.MeshResolverCollection;
import Brawler.MeshTypeID;

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
	void MeshResolverCollection::CreateMeshResolverForAIMesh(const aiMesh& mesh)
	{
		switch (GetMeshTypeID(mesh))
		{
		case Brawler::MeshTypeID::STATIC: [[likely]]
		{
			mStaticMeshResolverArr.emplace_back(mesh);
			break;
		}

		// TODO: Add support for skinned meshes. For now, we assert that the mesh is not
		// skinned and simply create a StaticMeshResolver.
		case Brawler::MeshTypeID::SKINNED: [[fallthrough]];
		default:
		{
			assert(false);
			std::unreachable();

			break;
		}
		}
	}

	void MeshResolverCollection::Update()
	{
		ForEachMeshResolver([]<typename T>(T& meshResolver)
		{
			meshResolver.Update();
		})
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
}