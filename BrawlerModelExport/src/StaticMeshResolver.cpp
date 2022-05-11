module;
#include <assimp/scene.h>

module Brawler.StaticMeshResolver;

namespace Brawler
{
	StaticMeshResolver::StaticMeshResolver(ImportedMesh&& mesh) :
		MeshResolverBase(std::move(mesh)),
		mVertexBuffer(mesh),
		mIndexBuffer(mesh)
	{}

	void StaticMeshResolver::UpdateIMPL()
	{
		// We actually do not have an IndexBuffer::Update() function, since index buffers
		// are simple enough to implement that we just do everything we need to in the
		// constructor of the IndexBuffer class.
		//
		// On the other hand, packing the VertexBuffer data does, in fact, take a significant
		// amount of CPU time. Thus, that gets postponed until the mesh resolver is updated
		// in order to better benefit from multithreading.

		mVertexBuffer.Update();
	}

	bool StaticMeshResolver::IsReadyForSerializationIMPL() const
	{
		// For now, this function will essentially always return true. However, this might
		// change in the future if packing vertex data is moved onto the GPU.

		return mVertexBuffer.IsReadyForSerialization();
	}
}