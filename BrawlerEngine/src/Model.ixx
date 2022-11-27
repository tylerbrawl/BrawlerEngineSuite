module;
#include <memory>
#include <span>
#include <vector>

export module Brawler.Model;
import Brawler.D3D12.BufferResource;
import Brawler.D3D12.BindlessSRVAllocation;
import Brawler.D3D12.StructuredBufferSubAllocation;
import Brawler.Mesh;
import Brawler.GPUSceneTypes;

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
		explicit Model(const std::span<Mesh> meshSpan);

		Model(const Model& rhs) = delete;
		Model& operator=(const Model& rhs) = delete;

		Model(Model&& rhs) noexcept = default;
		Model& operator=(Model&& rhs) noexcept = default;

		std::uint32_t GetMeshDescriptorBufferBindlessSRVIndex() const;

	private:
		void UpdateMeshDescriptorBufferData();

	private:
		std::unique_ptr<D3D12::BufferResource> mMeshDescriptorBufferPtr;
		D3D12::StructuredBufferSubAllocation<GPUSceneTypes::MeshDescriptor> mMeshDescriptorBufferSubAllocation;
		D3D12::BindlessSRVAllocation mMeshDescriptorBufferBindlessAllocation;
		std::vector<Mesh> mMeshArr;
	};
}