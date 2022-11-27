module;

export module Brawler.Mesh;
import Brawler.D3D12.StructuredBufferSubAllocation;
import Brawler.GPUSceneTypes;
import Brawler.IndexBuffer;
import Brawler.Math.MathTypes;

export namespace Brawler
{
	struct MeshInfo
	{
		D3D12::StructuredBufferSubAllocation<GPUSceneTypes::PackedStaticVertex> GlobalVertexBufferSubAllocation;
		IndexBuffer MeshIndexBuffer;
		Math::Float3 MinimumAABBPoint;
		Math::Float3 MaximumAABBPoint;
		MaterialDefinitionHandle HMaterial;
	};

	class Mesh
	{
	public:
		Mesh() = default;
		explicit Mesh(MeshInfo&& meshInfo);

		Mesh(const Mesh& rhs) = delete;
		Mesh& operator=(const Mesh& rhs) = delete;

		Mesh(Mesh&& rhs) noexcept = default;
		Mesh& operator=(Mesh&& rhs) noexcept = default;

		GPUSceneTypes::MeshDescriptor GetMeshDescriptor() const;

	private:
		MeshInfo mInfo;
	};
}