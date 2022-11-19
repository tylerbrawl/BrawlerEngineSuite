module;
#include <memory>

export module Brawler.Model;
import Brawler.D3D12.BufferResource;
import Brawler.D3D12.BindlessSRVAllocation;

/*
Each Model is represented by a StructuredBuffer<MeshDescriptor>. Each MeshDescriptor has
some AABBs, a MaterialDescriptor, and an index buffer. Each index buffer identifies a vertex
within the global vertex buffer.
*/

export namespace Brawler 
{
	class Model
	{
	public:
		Model() = default;

		Model(const Model& rhs) = delete;
		Model& operator=(const Model& rhs) = delete;

		Model(Model&& rhs) noexcept = default;
		Model& operator=(Model&& rhs) noexcept = default;

		std::uint32_t GetBindlessMeshDescriptorBufferSRVIndex() const;

	private:
		std::unique_ptr<D3D12::BufferResource> mMeshDescriptorBufferPtr;
		D3D12::BindlessSRVAllocation mMeshDescriptorBufferBindlessAllocation;
	};
}