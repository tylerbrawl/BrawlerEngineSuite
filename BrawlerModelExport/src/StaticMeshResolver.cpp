module;
#include <vector>
#include <span>
#include <cassert>
#include <assimp/scene.h>

module Brawler.StaticMeshResolver;

namespace Brawler
{
	StaticMeshResolver::StaticMeshResolver(ImportedMesh&& mesh) :
		MeshResolverBase(std::move(mesh)),
		mVertexBuffer(mesh.GetMesh()),
		mIndexBuffer(mesh.GetMesh())
	{}

	void StaticMeshResolver::UpdateIMPL()
	{
		// Packing the VertexBuffer takes a significant amount of CPU time, so we delay it until
		// the first update, rather than doing it in the constructor of the VertexBuffer class.
		//
		// Updating the index buffer is essentially a no-op for now. We leave that call here for
		// principle, but since it costs nothing, we don't bother creating any CPU jobs.
		
		mVertexBuffer.Update();
		mIndexBuffer.Update();
	}

	bool StaticMeshResolver::IsReadyForSerializationIMPL() const
	{
		return (mVertexBuffer.IsReadyForSerialization() && mIndexBuffer.IsReadyForSerialization());
	}
}