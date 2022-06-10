module;
#include <vector>
#include <span>
#include <variant>
#include <cassert>
#include <assimp/mesh.h>

export module Brawler.IndexBuffer;
import Brawler.NormalBoundingCones;

export namespace Brawler
{
	template <typename Vertex>
		requires HasPosition<Vertex>
	class IndexBuffer
	{
	public:
		explicit IndexBuffer(const aiMesh& mesh);

		IndexBuffer(const IndexBuffer& rhs) = delete;
		IndexBuffer& operator=(const IndexBuffer& rhs) = delete;

		IndexBuffer(IndexBuffer&& rhs) noexcept = default;
		IndexBuffer& operator=(IndexBuffer&& rhs) noexcept = default;

		void PrepareTrianglesForGrouping(const std::span<const Vertex> vertexSpan);
		void GroupTriangles();

		bool IsReadyForSerialization() const;

	private:
		/// <summary>
		/// The first element in this std::variant is immediately initialized in the constructor.
		/// 
		/// The second element contains an array of NormalBoundingConeTriangleGroup instances.
		/// These represent triangles grouped together alongside their respective normal bounding
		/// cone, a la "Optimizing the Graphics Pipeline with Compute."
		/// </summary>
		std::variant<std::vector<std::uint16_t>, std::vector<NormalBoundingConeTriangleGroup<Vertex>>> mIndexVariant;

		NormalBoundingConeTriangleGrouper<Vertex> mTriangleGrouper;
	};
}

// --------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename Vertex>
		requires HasPosition<Vertex>
	IndexBuffer<Vertex>::IndexBuffer(const aiMesh& mesh) :
		mIndexVariant(std::vector<std::uint16_t>{}),
		mTriangleGrouper()
	{
		// We should have guaranteed that the mesh was triangulated in the SceneLoader class.
		assert(mesh.mPrimitiveTypes == aiPrimitiveType::aiPrimitiveType_TRIANGLE && "ERROR: A mesh was not properly triangulated during loading!");

		std::vector<std::uint16_t>& indexArr{ std::get<0>(mIndexVariant) };
		indexArr.reserve(static_cast<std::size_t>(mesh.mNumFaces) * 3);

		const std::span<const aiFace> meshFaceArr{ mesh.mFaces, static_cast<std::size_t>(mesh.mNumFaces) };
		for (const auto& face : meshFaceArr)
		{
			// We only support 16-bit indices (for now).

			assert(face.mIndices[0] <= std::numeric_limits<std::uint16_t>::max());
			indexArr.push_back(static_cast<std::uint16_t>(face.mIndices[0]));

			assert(face.mIndices[1] <= std::numeric_limits<std::uint16_t>::max());
			indexArr.push_back(static_cast<std::uint16_t>(face.mIndices[1]));

			assert(face.mIndices[2] <= std::numeric_limits<std::uint16_t>::max());
			indexArr.push_back(static_cast<std::uint16_t>(face.mIndices[2]));
		}
	}

	template <typename Vertex>
		requires HasPosition<Vertex>
	void IndexBuffer<Vertex>::PrepareTrianglesForGrouping(const std::span<const Vertex> vertexSpan)
	{
		// On the update of the IndexBuffer instance, we want to group the triangles together so
		// that they can easily be culled at runtime by checking a normal bounding cone.

		const std::span<const std::uint16_t> indexSpan{ std::get<0>(mIndexVariant) };
		mTriangleGrouper.PrepareTriangleBuckets(vertexSpan, indexSpan);
	}

	template <typename Vertex>
		requires HasPosition<Vertex>
	void IndexBuffer<Vertex>::GroupTriangles()
	{
		mTriangleGrouper.SolveNormalBoundingCones();
		mIndexVariant = mTriangleGrouper.GetNormalBoundingConeTriangleGroups();
	}

	template <typename Vertex>
		requires HasPosition<Vertex>
	bool IndexBuffer<Vertex>::IsReadyForSerialization() const
	{
		// An IndexBuffer instance is ready to be serialized when all of the triangles have been
		// grouped together appropriately, and the normal bounding cones for each of these groups
		// has been found. We know this is the case if mIndexVariant currently holds these groups.

		return (mIndexVariant.index() == 1);
	}
}