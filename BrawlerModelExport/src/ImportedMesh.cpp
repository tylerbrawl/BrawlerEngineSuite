module;
#include <span>
#include <cassert>
#include <assimp/scene.h>

module Brawler.ImportedMesh;

namespace Brawler
{
	ImportedMesh::ImportedMesh(const aiMesh& mesh, const aiScene& owningScene) :
		mAIMeshPtr(&mesh),
		mAIScenePtr(&owningScene)
	{}

	const aiMesh& ImportedMesh::GetMesh() const
	{
		assert(mAIMeshPtr != nullptr);
		return *mAIMeshPtr;
	}

	const aiScene& ImportedMesh::GetOwningScene() const
	{
		assert(mAIScenePtr != nullptr);
		return *mAIScenePtr;
	}

	const aiMaterial& ImportedMesh::GetMeshMaterial() const
	{
		assert(mAIScenePtr != nullptr && mAIMeshPtr != nullptr);
		
		const std::span<const aiMaterial*> materialPtrSpan{ const_cast<const aiMaterial**>(mAIScenePtr->mMaterials), mAIScenePtr->mNumMaterials };
		return *(materialPtrSpan[mAIMeshPtr->mMaterialIndex]);
	}
}