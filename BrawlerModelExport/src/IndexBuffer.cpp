module;
#include <vector>
#include <span>
#include <variant>
#include <cassert>
#include <assimp/mesh.h>

module Brawler.IndexBuffer;

namespace Brawler
{
	IndexBuffer::IndexBuffer(const aiMesh& mesh) :
		mIndexArr()
	{
		// We should have guaranteed that the mesh was triangulated in the SceneLoader class.
		assert(mesh.mPrimitiveTypes == aiPrimitiveType::aiPrimitiveType_TRIANGLE && "ERROR: A mesh was not properly triangulated during loading!");

		mIndexArr.reserve(static_cast<std::size_t>(mesh.mNumFaces) * 3);

		const std::span<const aiFace> meshFaceArr{ mesh.mFaces, static_cast<std::size_t>(mesh.mNumFaces) };
		for (const auto& face : meshFaceArr)
		{
			mIndexArr.push_back(face.mIndices[0]);
			mIndexArr.push_back(face.mIndices[1]);
			mIndexArr.push_back(face.mIndices[2]);
		}
	}

	void IndexBuffer::Update()
	{}

	bool IndexBuffer::IsReadyForSerialization() const
	{
		return true;
	}
}