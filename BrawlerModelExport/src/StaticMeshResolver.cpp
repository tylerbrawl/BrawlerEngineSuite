module;
#include <vector>
#include <span>
#include <cassert>
#include <assimp/scene.h>

module Brawler.StaticMeshResolver;
import Brawler.JobSystem;
import Brawler.Math.AABB;

namespace Brawler
{
	StaticMeshResolver::StaticMeshResolver(std::unique_ptr<ImportedMesh>&& meshPtr) :
		MeshResolverBase(std::move(meshPtr)),
		mVertexBuffer(GetImportedMesh()),
		mIndexBuffer(GetImportedMesh())
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

	StaticMeshResolver::SerializedMeshData StaticMeshResolver::SerializeMeshDataIMPL() const
	{
		struct VertexBufferJobInfo
		{
			DirectX::XMFLOAT3 AABBMinPoint;
			DirectX::XMFLOAT3 AABBMaxPoint;
			std::uint32_t VertexCount;
			std::uint64_t VertexBufferFilePathHash;
		};

		struct IndexBufferJobInfo
		{
			std::uint32_t IndexCount;
			std::uint64_t IndexBufferFilePathHash;
		};

		Brawler::JobGroup meshDataSerializationGroup{};
		meshDataSerializationGroup.Reserve(2);

		VertexBufferJobInfo vbInfo{};

		meshDataSerializationGroup.AddJob([this, &vbInfo] ()
		{
			const Math::AABB& objectSpaceBoundingBox{ mVertexBuffer.GetBoundingBox() };
			vbInfo.AABBMinPoint = objectSpaceBoundingBox.GetMinimumBoundingPoint();
			vbInfo.AABBMaxPoint = objectSpaceBoundingBox.GetMaximumBoundingPoint();

			assert(mVertexBuffer.GetVertexCount() <= std::numeric_limits<std::uint32_t>::max());
			vbInfo.VertexCount = static_cast<std::uint32_t>(mVertexBuffer.GetVertexCount());

			vbInfo.VertexBufferFilePathHash = mVertexBuffer.SerializeVertexBuffer();
		});

		IndexBufferJobInfo ibInfo{};

		meshDataSerializationGroup.AddJob([this, &ibInfo] ()
		{
			assert(mIndexBuffer.GetIndexCount() <= std::numeric_limits<std::uint32_t>::max());
			ibInfo.IndexCount = static_cast<std::uint32_t>(mIndexBuffer.GetIndexCount());

			ibInfo.IndexBufferFilePathHash = mIndexBuffer.SerializeIndexBuffer();
		});

		meshDataSerializationGroup.ExecuteJobs();

		return SerializedMeshData{
			.AABBMinPoint{std::move(vbInfo.AABBMinPoint)},
			.VertexCount = vbInfo.VertexCount,
			.AABBMaxPoint{std::move(vbInfo.AABBMaxPoint)},
			.IndexCount = ibInfo.IndexCount,
			.VertexBufferFilePathHash = vbInfo.VertexBufferFilePathHash,
			.IndexBufferFilePathHash = ibInfo.IndexBufferFilePathHash
		};
	}
}