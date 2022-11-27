module;
#include <cstddef>
#include <span>

export module Brawler.Mesh;
import Brawler.D3D12.StructuredBufferSubAllocation;
import Brawler.GPUSceneTypes;
import Brawler.MeshBuilder;

export namespace Brawler
{
	class Mesh
	{
	public:
		Mesh() = default;
		explicit Mesh(MeshBuilder&& builder);

		Mesh(const Mesh& rhs) = delete;
		Mesh& operator=(const Mesh& rhs) = delete;

		Mesh(Mesh&& rhs) noexcept = default;
		Mesh& operator=(Mesh&& rhs) noexcept = default;

	private:
		D3D12::StructuredBufferSubAllocation<GPUSceneTypes::PackedStaticVertex> mGlobalVertexBufferSubAllocation;

	};
}