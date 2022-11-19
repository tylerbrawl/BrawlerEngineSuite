module;
#include <cstddef>
#include <span>

export module Brawler.Mesh;
import Brawler.D3D12.StructuredBufferSubAllocation;
import Brawler.GPUSceneTypes;

export namespace Brawler
{
	class Mesh
	{
	public:
		Mesh() = default;
		explicit Mesh(const std::size_t numVertices);

		Mesh(const Mesh& rhs) = delete;
		Mesh& operator=(const Mesh& rhs) = delete;

		Mesh(Mesh&& rhs) noexcept = default;
		Mesh& operator=(Mesh&& rhs) noexcept = default;

	private:
		D3D12::StructuredBufferSubAllocation<GPUSceneTypes::PackedStaticVertex> mGlobalVertexBufferSubAllocation;
		D3D12::StructuredBufferSubAllocation<GPUSceneTypes::MeshDescriptor> mMeshDescriptorSubAllocation;
	};
}