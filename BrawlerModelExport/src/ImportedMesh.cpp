module;
#include <span>
#include <cassert>
#include <assimp/scene.h>

module Brawler.ImportedMesh;

namespace Brawler
{
	ImportedMesh::ImportedMesh(const aiMesh& mesh, const std::uint32_t meshIDForLOD, LODScene&& owningScene) :
		mAIMeshPtr(&mesh),
		mOwningScene(std::move(owningScene)),
		mMeshIDForLOD(meshIDForLOD)
	{}

	const aiMesh& ImportedMesh::GetMesh() const
	{
		assert(mAIMeshPtr != nullptr);
		return *mAIMeshPtr;
	}

	const aiScene& ImportedMesh::GetOwningScene() const
	{
		return mOwningScene.GetScene();
	}

	std::uint32_t ImportedMesh::GetMeshIDForLOD() const
	{
		return mMeshIDForLOD;
	}

	const aiMaterial& ImportedMesh::GetMeshMaterial() const
	{
		assert(mAIMeshPtr != nullptr);
		
		const aiScene& assimpScene{ mOwningScene.GetScene() };
		const std::span<const aiMaterial*> materialPtrSpan{ const_cast<const aiMaterial**>(assimpScene.mMaterials), assimpScene.mNumMaterials };

		return *(materialPtrSpan[mAIMeshPtr->mMaterialIndex]);
	}

	LODScene ImportedMesh::GetLODScene() const
	{
		return mOwningScene;
	}
}