module;
#include <span>
#include <vector>

export module Brawler.MeshBuilder;
import Brawler.GPUSceneTypes;
import Brawler.MaterialDefinitionHandle;
import Brawler.Math.MathTypes;

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

		void SetVertexBufferSize(const std::size_t numVertices);

		std::span<GPUSceneTypes::PackedStaticVertex> GetVertexSpan();
		std::span<const GPUSceneTypes::PackedStaticVertex> GetVertexSpan() const;

		// Although the minimum and maximum AABB points can be calculated automatically
		// based on the values specified in mVertexArr, we have the user manually specify
		// them for the sake of performance. Specifically, a "real" mesh system
		// would compute the minimum and maximum AABB points in an offline process and
		// store these in the file.
		void SetMinimumAABBPoint(Math::Float3 minAABBPoint);
		Math::Float3 GetMaximumAABBPoint() const;

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