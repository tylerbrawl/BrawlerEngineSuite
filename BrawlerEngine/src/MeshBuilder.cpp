module;
#include <span>
#include <stdexcept>
#include <vector>

module Brawler.MeshBuilder;
import Brawler.D3D12.StructuredBufferSubAllocation;
import Brawler.GPUSceneManager;
import Brawler.GPUSceneBufferID;
import Brawler.D3D12.BufferResource;
import Brawler.Application;
import Brawler.D3D12.Renderer;
import Brawler.GPUSceneUpdateRenderModule;
import Brawler.GPUSceneBufferUpdateOperation;

namespace Brawler
{
	Mesh MeshBuilder::CreateMesh()
	{
		// Reserve a segment from the global vertex buffer. If we can't do this, then we've run
		// out of global vertex buffer memory.
		D3D12::StructuredBufferSubAllocation<GPUSceneTypes::PackedStaticVertex> globalVertexBufferSubAllocation{ mVertexArr.size() };

		{
			const bool wasReservationSuccessful = GPUSceneManager::GetInstance().GetGPUSceneBufferResource<GPUSceneBufferID::GLOBAL_VERTEX_BUFFER>().AssignReservation(globalVertexBufferSubAllocation);

			if (!wasReservationSuccessful) [[unlikely]]
				throw std::runtime_error{ "ERROR: The global vertex buffer has ran out of memory!" };
		}

		// Submit an update for the next frame to write the new vertices into the assigned segment
		// of the global vertex buffer.
		{
			GPUSceneBufferUpdateOperation<GPUSceneBufferID::GLOBAL_VERTEX_BUFFER> vertexBufferUpdateOperation{ globalVertexBufferSubAllocation.GetBufferCopyRegion() };
			vertexBufferUpdateOperation.SetUpdateSourceData(GetVertexSpan());

			Brawler::GetRenderer().GetRenderModule<GPUSceneUpdateRenderModule>().ScheduleGPUSceneBufferUpdateForNextFrame(std::move(vertexBufferUpdateOperation));
		}

		// Now that we will be using the vertices through the global vertex buffer, our current index
		// buffer indices are invalid. To correct this, we simply offset each index by the element offset
		// from the start of the global vertex buffer to the start of the segment we were assigned.
		const std::uint32_t indexBufferElementOffset = (globalVertexBufferSubAllocation.GetOffsetFromBufferStart() / sizeof(std::uint32_t));

		for (auto& index : mIndexArr)
			index += indexBufferElementOffset;

		// Next, create an IndexBuffer instance for the Mesh we are creating. IndexBuffer::SetIndices()
		// doesn't actually store any index values in CPU data; rather, it creates a generic per-frame
		// buffer update which will write our index values into GPU memory. Each IndexBuffer instance
		// has a BufferResource instance, which is where the indices are written into.
		IndexBuffer meshIndexBuffer{ mIndexArr.size() };
		meshIndexBuffer.SetIndices(GetIndexSpan());

		// We have finally prepared all of the data necessary to create the Mesh instance.
		MeshInfo meshInfo{
			.GlobalVertexBufferSubAllocation{ std::move(globalVertexBufferSubAllocation) },
			.MeshIndexBuffer{ std::move(meshIndexBuffer) },
			.MinimumAABBPoint{ GetMinimumAABBPoint() },
			.MaximumAABBPoint{ GetMaximumAABBPoint() },
			.HMaterial{ std::move(mHMaterial) }
		};

		return Mesh{ std::move(meshInfo) };
	}

	void MeshBuilder::SetVertexBufferSize(const std::size_t numVertices)
	{
		mVertexArr.resize(numVertices);
	}

	std::span<GPUSceneTypes::PackedStaticVertex> MeshBuilder::GetVertexSpan()
	{
		return { mVertexArr };
	}

	std::span<const GPUSceneTypes::PackedStaticVertex> MeshBuilder::GetVertexSpan() const
	{
		return { mVertexArr };
	}

	void MeshBuilder::SetMinimumAABBPoint(Math::Float3 minAABBPoint)
	{
		mMinAABBPoint = std::move(minAABBPoint);
	}

	Math::Float3 MeshBuilder::GetMinimumAABBPoint() const
	{
		return mMinAABBPoint;
	}

	void MeshBuilder::SetMaximumAABBPoint(Math::Float3 maxAABBPoint)
	{
		mMaxAABBPoint = std::move(maxAABBPoint);
	}

	Math::Float3 MeshBuilder::GetMaximumAABBPoint() const
	{
		return mMaxAABBPoint;
	}

	void MeshBuilder::SetIndexBufferSize(const std::size_t numIndices)
	{
		mIndexArr.resize(numIndices);
	}

	std::span<std::uint32_t> MeshBuilder::GetIndexSpan()
	{
		return { mIndexArr };
	}

	std::span<const std::uint32_t> MeshBuilder::GetIndexSpan() const
	{
		return { mIndexArr };
	}

	void MeshBuilder::SetMaterialDefinitionHandle(MaterialDefinitionHandle&& hMaterial)
	{
		mHMaterial = std::move(hMaterial);
	}
}