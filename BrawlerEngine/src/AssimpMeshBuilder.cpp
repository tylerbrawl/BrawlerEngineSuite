module;
#include <cassert>
#include <ranges>
#include <cmath>
#include <span>
#include <bit>
#include <algorithm>
#include <assimp/mesh.h>

module Brawler.AssimpSceneLoader;
import Brawler.GPUSceneTypes;
import Brawler.Math.MathTypes;
import Brawler.Math.MathConstants;

namespace
{
	Brawler::Math::Float2 OctahedronWrap(const Brawler::Math::Float2 v)
	{
		Brawler::Math::Float2 wrappedVector{ 1.0f - Brawler::Math::Float2{ std::abs(v.GetY()), std::abs(v.GetX()) } };
		const Brawler::Math::Float2 scaleVector{ v.GetX() >= 0.0f ? 1.0f : -1.0f, v.GetY() >= 0.0f ? 1.0f : -1.0f };

		return (wrappedVector * scaleVector);
	}

	Brawler::Math::Float2 PackNormal(const Brawler::Math::Float3 unpackedNormal)
	{
		const float absComponentSum = (std::abs(unpackedNormal.GetX()) + std::abs(unpackedNormal.GetY()) + std::abs(unpackedNormal.GetZ()));
		Brawler::Math::Float3 scaledNormal{ unpackedNormal / absComponentSum };

		if (scaledNormal.GetZ() < 0.0f)
		{
			const Brawler::Math::Float2 wrappedNormal{ OctahedronWrap(Brawler::Math::Float2{ scaledNormal.GetX(), scaledNormal.GetY() }) };

			scaledNormal.GetX() = wrappedNormal.GetX();
			scaledNormal.GetY() = wrappedNormal.GetY();
		}

		Brawler::Math::Float2 packedNormal{ scaledNormal.GetX(), scaledNormal.GetY() };

		// Encode the values in the range [0, 1].
		packedNormal *= 0.5f;
		packedNormal += 0.5f;

		return packedNormal;
	}

	struct TangentFrame
	{
		Brawler::Math::Float3 Tangent;
		Brawler::Math::Float3 Bitangent;
		Brawler::Math::Float3 Normal;
	};

	float PackTangentFrame(const TangentFrame& tangentFrame)
	{
		const Brawler::Math::Float2 packedNormal{ PackNormal(tangentFrame.Normal) };

		const Brawler::Math::Float3 tangent_b = (std::abs(tangentFrame.Normal.GetX()) > std::abs(tangentFrame.Normal.GetZ()) ?
			Brawler::Math::Float3{ -(tangentFrame.Normal.GetY()), tangentFrame.Normal.GetX(), 0.0f } :
			Brawler::Math::Float3{ 0.0f, -(tangentFrame.Normal.GetZ()), tangentFrame.Normal.GetY() }
		);

		const Brawler::Math::Float3 normalizedTangent_b{ tangent_b.Normalize() };

		// Find the angle which we need to rotate tangent_b about the normal to get
		// the tangent vector. We do this by determining the angle between tangent_b
		// and tangent and then rotating tangent_b about the normal by this angle and
		// checking the resulting vector. If we get the tangent vector back, then this
		// is our angle; otherwise, we need to use (2Pi - angle).
		const float angleBetweenTangents = std::acosf(normalizedTangent_b.Dot(tangentFrame.Tangent));

		const Brawler::Math::Quaternion rotationQuaternion{ tangentFrame.Normal, angleBetweenTangents };
		const Brawler::Math::Float3 rotatedTangent_b{ rotationQuaternion.RotateVector(normalizedTangent_b) };

		static constexpr float EPSILON = 0.00001f;

		// If (a . b) = 1, then the angle between the vectors a and b must be zero.
		const float absCosineAngleDifference = std::abs(1.0f - rotatedTangent_b.Dot(tangentFrame.Tangent));

		const float rotationRadians = (absCosineAngleDifference < EPSILON ? angleBetweenTangents : (Brawler::Math::TWO_PI - angleBetweenTangents));

		// For the encoded normal, we need to map values from the range [0, 1] to the
		// range [0, 255].
		//
		// For the rotation in radians, we need to map values from the range [0, 2Pi]
		// to the range [0, 255].
		static constexpr float ENCODED_NORMAL_SCALE_VALUE = 255.0f;
		static constexpr float ROTATION_SCALE_VALUE = (255.0f / Brawler::Math::TWO_PI);

		std::uint32_t packedTangentFrame = 0;

		{
			const Brawler::Math::Float2 mappedPackedNormal{ packedNormal * ENCODED_NORMAL_SCALE_VALUE };

			packedTangentFrame |= (static_cast<std::uint32_t>(mappedPackedNormal.GetX()) << 24);
			packedTangentFrame |= (static_cast<std::uint32_t>(mappedPackedNormal.GetY()) << 16);
		}

		{
			const float mappedRotationRadians = (rotationRadians * ROTATION_SCALE_VALUE);
			packedTangentFrame |= (static_cast<std::uint32_t>(mappedRotationRadians) << 8);
		}

		return std::bit_cast<float>(packedTangentFrame);
	}
}

namespace Brawler
{
	AssimpMeshBuilder::AssimpMeshBuilder(const aiMesh& mesh) :
		mBuilder(),
		mMeshPtr(&mesh)
	{
		assert(mesh.HasPositions() && "ERROR: An attempt was made to construct a Brawler::Mesh from an aiMesh instance with no vertex positions!");
		assert(mesh.HasFaces() && "ERROR: An attempt was made to construct a Brawler::Mesh from an aiMesh instance with no faces!");
		assert(mesh.HasNormals() && "ERROR: An attempt was made to construct a Brawler::Mesh from an aiMesh instance with no normals!");
		assert(mesh.HasTangentsAndBitangents() && "ERROR: An attempt was made to construct a Brawler::Mesh from an aiMesh instance with no tangents or bitangents!");
		assert(mesh.HasTextureCoords() && "ERROR: An attempt was made to construct a Brawler::Mesh from an aiMesh instance with no UV texture coordinates!");
	}

	void AssimpMeshBuilder::InitializeMeshData()
	{
		InitializeVertexBufferData();
		InitializeIndexBufferData();
		InitializeAABBData();
	}

	void AssimpMeshBuilder::SetMaterialDefinitionHandle(MaterialDefinitionHandle&& hMaterial)
	{
		mBuilder.SetMaterialDefinitionHandle(std::move(hMaterial));
	}

	Mesh AssimpMeshBuilder::CreateMesh()
	{
		return mBuilder.CreateMesh();
	}

	void AssimpMeshBuilder::InitializeVertexBufferData()
	{
		const std::size_t numVertices = mMeshPtr->mNumVertices;
		mBuilder.SetVertexBufferSize(numVertices);

		const std::span<GPUSceneTypes::PackedStaticVertex> destVertexSpan{ mBuilder.GetVertexSpan() };

		const std::span<const aiVector3D> vertexPositionsSpan{ mMeshPtr->mVertices, numVertices };
		const std::span<const aiVector3D> normalsSpan{ mMeshPtr->mNormals, numVertices };
		const std::span<const aiVector3D> tangentsSpan{ mMeshPtr->mTangents, numVertices };
		const std::span<const aiVector3D> bitangentsSpan{ mMeshPtr->mBitangents, numVertices };
		const std::span<const aiVector3D> uvCoordsSpan{ mMeshPtr->mTextureCoords[0], numVertices };

		for (auto& [destVertex, currPos, currNormal, currTangent, currBitangent, currUVCoords] : std::views::zip(
			destVertexSpan,
			vertexPositionsSpan,
			normalsSpan,
			tangentsSpan,
			bitangentsSpan,
			uvCoordsSpan
		))
		{
			const TangentFrame tangentFrame{
				.Tangent{ Math::Float3{ currTangent.x, currTangent.y, currTangent.z }.Normalize() },
				.Bitangent{ Math::Float3{ currBitangent.x, currBitangent.y, currBitangent.z }.Normalize() },
				.Normal{ Math::Float3{ currNormal.x, currNormal.y, currNormal.z }.Normalize() }
			};

			const float crustyTangentFrame = PackTangentFrame(tangentFrame);

			destVertex = GPUSceneTypes::PackedStaticVertex{
				.PositionAndTangentFrame{ currPos.x, currPos.y, currPos.z, crustyTangentFrame },
				.UVCoords{ currUVCoords.x, currUVCoords.y }
			};
		}
	}

	void AssimpMeshBuilder::InitializeIndexBufferData()
	{
		// Assimp supports meshes composed of many different types of primitives, but we
		// are concerned with only triangular meshes. (Indeed, we ask Assimp to triangulate
		// complex meshes as necessary to reduce them to only triangles.)
		const std::size_t numTriangles = mMeshPtr->mNumFaces;
		const std::size_t numIndices = (numTriangles * 3);

		mBuilder.SetIndexBufferSize(numIndices);

		const auto destTriangleRange{ mBuilder.GetIndexSpan() | std::views::chunk(3) };
		const std::span<const aiFace> faceSpan{ mMeshPtr->mFaces, numTriangles };

		for (auto& [destTriangle, srcFace] : std::views::zip(destTriangleRange, faceSpan))
		{
			assert(srcFace.mNumIndices == 3 && "ERROR: A non-triangle primitive was detected within an aiMesh during Assimp scene loading!");
			const std::span<const std::uint32_t> srcIndexSpan{ srcFace.mIndices, srcFace.mNumIndices };

			std::ranges::copy(srcIndexSpan, destTriangle.begin());
		}
	}

	void AssimpMeshBuilder::InitializeAABBData()
	{
		// Assimp actually calculates AABB points for us automatically, so we can just use
		// those instead of calculating them ourselves.
		const aiAABB& meshAABB{ mMeshPtr->mAABB };

		mBuilder.SetMinimumAABBPoint(Math::Float3{ meshAABB.mMin.x, meshAABB.mMin.y, meshAABB.mMin.z });
		mBuilder.SetMaximumAABBPoint(Math::Float3{ meshAABB.mMax.x, meshAABB.mMax.y, meshAABB.mMax.z });
	}
}