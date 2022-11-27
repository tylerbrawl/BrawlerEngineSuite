module;
#include <span>
#include <vector>

export module Brawler.MeshBuilder;
import Brawler.GPUSceneTypes;
import Brawler.MaterialDefinitionHandle;
import Brawler.Math.MathTypes;
import Brawler.Mesh;

export namespace Brawler
{
	class MeshBuilder
	{
	public:
		MeshBuilder() = default;

		MeshBuilder(const MeshBuilder& rhs) = delete;
		MeshBuilder& operator=(const MeshBuilder& rhs) = delete;

		MeshBuilder(MeshBuilder&& rhs) noexcept = delete;
		MeshBuilder& operator=(MeshBuilder&& rhs) noexcept = delete;

		/// <summary>
		/// Creates a Mesh instance using the parameters defined by making calls to
		/// other functions of this MeshBuilder instance. Specifically, doing this will
		/// perform the following series of tasks:
		/// 
		/// 1. Reserve a segment of the global vertex buffer for this mesh, and write the
		///    vertices into said segment of GPU memory as a generic per-frame buffer update.
		/// 
		/// 2. Create an IndexBuffer for this mesh, and write the indices into the corresponding
		///    buffer in GPU memory as a generic per-frame buffer update.
		/// 
		/// It is assumed that the following parameters have been defined in previous calls
		/// to member functions of this MeshBuilder instance:
		/// 
		///   - Vertex Buffer Data (Use MeshBuilder::SetVertexBufferSize() and 
		///     MeshBuilder::GetVertexSpan())
		/// 
		///   - Index Buffer Data (Use MeshBuilder::SetIndexBufferSize() and
		///     MeshBuilder::GetIndexSpan())
		/// 
		///   - Maximum AABB Point (Use MeshBuilder::SetMaximumAABBPoint())
		/// 
		///   - Minimum AABB Point (Use MeshBuilder::SetMinimumAABBPoint())
		/// 
		///   - Material Definition (Use MeshBuilder::SetMaterialDefinitionHandle())
		/// 
		/// Checks are *NOT* done to ensure that these parameters have been set, even when
		/// Util::General::IsDebugModeEnabled() returns true. (TODO: Add these checks in Debug
		/// builds!)
		/// 
		/// Generally speaking, it is unlikely that Mesh creation would fail if all of the
		/// parameters have been defined. Thus, exceptions are used in the case of a failure.
		/// Specifically, an exception is thrown immediately if there is not enough contiguous 
		/// memory available in the global vertex buffer for this Mesh instance. Likewise, a GPU 
		/// out-of-memory exception is thrown during FrameGraph building if the IndexBuffer 
		/// could not be given an allocation within an ID3D12Heap.
		/// 
		/// *WARNING*: After calling this function, the MeshBuilder instance is left in an
		/// indeterminate state. The behavior is *UNDEFINED* if an attempt is made to call any
		/// further member functions on said instance after a call to MeshBuilder::CreateMesh()
		/// returns.
		/// </summary>
		/// <returns>
		/// If the function succeeds, then it returns a Mesh instance using the parameters set
		/// in previous calls to member functions of this MeshBuilder instance.
		/// 
		/// In the highly unlikely scenario that the function fails, an exception is thrown.
		/// </returns>
		Mesh CreateMesh();

		void SetVertexBufferSize(const std::size_t numVertices);

		std::span<GPUSceneTypes::PackedStaticVertex> GetVertexSpan();
		std::span<const GPUSceneTypes::PackedStaticVertex> GetVertexSpan() const;

		// Although the minimum and maximum AABB points can be calculated automatically
		// based on the values specified in mVertexArr, we have the user manually specify
		// them for the sake of performance. Specifically, a "real" mesh system
		// would compute the minimum and maximum AABB points in an offline process and
		// store these in a file.

		void SetMinimumAABBPoint(Math::Float3 minAABBPoint);
		Math::Float3 GetMinimumAABBPoint() const;

		void SetMaximumAABBPoint(Math::Float3 maxAABBPoint);
		Math::Float3 GetMaximumAABBPoint() const;

		void SetIndexBufferSize(const std::size_t numIndices);

		std::span<std::uint32_t> GetIndexSpan();
		std::span<const std::uint32_t> GetIndexSpan() const;

		void SetMaterialDefinitionHandle(MaterialDefinitionHandle&& hMaterial);

	private:
		std::vector<GPUSceneTypes::PackedStaticVertex> mVertexArr;
		std::vector<std::uint32_t> mIndexArr;
		Math::Float3 mMinAABBPoint;
		Math::Float3 mMaxAABBPoint;
		MaterialDefinitionHandle mHMaterial;
	};
}