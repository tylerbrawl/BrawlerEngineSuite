module;
#include <cassert>
#include <vector>
#include <span>
#include <assimp/mesh.h>

module Brawler.IndexBuffer;

namespace Brawler
{
	IndexBuffer::IndexBuffer(const aiMesh& mesh) :
		mIndices()
	{
		// We should have guaranteed that the mesh was triangulated in the SceneLoader class.
		assert(mesh.mPrimitiveTypes == aiPrimitiveType::aiPrimitiveType_TRIANGLE && "ERROR: A mesh was not properly triangulated during loading!");

		mIndices.reserve(static_cast<std::size_t>(mesh.mNumFaces) * 3);

		const std::span<const aiFace> meshFaceArr{ mesh.mFaces, static_cast<std::size_t>(mesh.mNumFaces) };
		for (const auto& face : meshFaceArr)
		{
			// We do an assert within the StaticVertexBuffer class that our vertices can be
			// indexed with 16-bit indices, so this conversion is safe.

			mIndices.push_back(static_cast<std::uint16_t>(face.mIndices[0]));
			mIndices.push_back(static_cast<std::uint16_t>(face.mIndices[1]));
			mIndices.push_back(static_cast<std::uint16_t>(face.mIndices[2]));
		}
	}
}