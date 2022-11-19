module;
#include <span>
#include <vector>

export module Brawler.MeshBuilder;
import Brawler.GPUSceneTypes;

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

		void SetIndexBufferSize(const std::size_t numIndices);

		std::span<std::uint32_t> GetIndexSpan();
		std::span<const std::uint32_t> GetIndexSpan() const;



	private:
		std::vector<GPUSceneTypes::PackedStaticVertex> mVertexArr;
		std::vector<std::uint32_t> mIndexArr;
	};
}