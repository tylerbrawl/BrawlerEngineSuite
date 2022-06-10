module;
#include <vector>
#include <span>
#include <cassert>
#include <assimp/scene.h>

module Brawler.StaticMeshResolver;
import Brawler.JobSystem;

namespace Brawler
{
	StaticMeshResolver::StaticMeshResolver(ImportedMesh&& mesh) :
		MeshResolverBase(std::move(mesh)),
		mVertexBuffer(mesh.GetMesh()),
		mIndexBuffer(mesh.GetMesh()),
		mNumUpdateCalls(0)
	{}

	void StaticMeshResolver::UpdateIMPL()
	{
		// Packing the VertexBuffer takes a significant amount of CPU time, but creating the
		// normal bounding cones takes an even longer amount of CPU time. In addition, the
		// first Update() of this MeshResolver instance will also be generating RenderPasses
		// for resolving the textures of its I_MaterialDefinition.
		//
		// What tends to happen is after the RenderPass instances get generated, the GPU will
		// continue to do work, and the threads will record the D3D12 command lists. However,
		// recording the command lists takes significantly less time compared to how long it
		// tends to take the GPU to resolve all of the textures for all of the I_MaterialDefinition
		// instances across every MeshResolver. This leads to all of the CPU threads idling until
		// the FrameGraph signals that the GPU has executed all of the texture resolution
		// RenderPass commands.
		//
		// To alleviate this, we can postpone the creation of the normal bounding cones until
		// the second call to this function, after the GPU has already been given some work.
		// This will prevent the CPU threads from sitting idle while the GPU resolves the
		// required textures.
		
		mVertexBuffer.Update();
		UpdateIndexBuffer();

		++mNumUpdateCalls;
	}

	bool StaticMeshResolver::IsReadyForSerializationIMPL() const
	{
		return (mVertexBuffer.IsReadyForSerialization() && mIndexBuffer.IsReadyForSerialization());
	}

	void StaticMeshResolver::UpdateIndexBuffer()
	{
		// On the first call to this function, prepare the triangles for being grouped.
		if (mNumUpdateCalls == 0)
		{
			const std::span<const UnpackedStaticVertex> vertexSpan{ mVertexBuffer.GetUnpackedVertexSpan() };
			mIndexBuffer.PrepareTrianglesForGrouping(vertexSpan);
		}

		// On the second call to this function, after we have already generated useful work for
		// the GPU, begin actually grouping the triangles. See the comments in
		// StaticMeshResolver::UpdateIMPL() as to why splitting these tasks into two leads to
		// better system utilization.
		else if (mNumUpdateCalls == 1)
			mIndexBuffer.GroupTriangles();
	}
}