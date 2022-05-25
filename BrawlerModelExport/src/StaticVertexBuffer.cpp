module;
#include <cassert>
#include <vector>
#include <limits>
#include <stdexcept>
#include <span>
#include <format>
#include <assimp/mesh.h>
#include <DirectXMath/DirectXMath.h>

module Brawler.StaticVertexBuffer;

namespace
{
	static constexpr DirectX::XMFLOAT3 AABB_MINIMUM_POINT_INIT{ std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
	static constexpr DirectX::XMFLOAT3 AABB_MAXIMUM_POINT_INIT{ std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min() };

	struct TangentFrame
	{
		DirectX::XMFLOAT2 OctahedronEncodedNormal;
		float RotationAngle;
	};

	DirectX::XMVECTOR XM_CALLCONV OctahedronWrap(DirectX::FXMVECTOR wrapVector)
	{
		// Save and remove the z-component.
		const float zComponent = DirectX::XMVectorGetZ(wrapVector);
		DirectX::XMVECTOR newWrapVector = DirectX::XMVectorPermute<DirectX::XM_PERMUTE_0X, DirectX::XM_PERMUTE_0Y, DirectX::XM_PERMUTE_1Z, DirectX::XM_PERMUTE_1W>(wrapVector, DirectX::XMVectorZero());

		if (zComponent >= 0.0f)
			return newWrapVector;

		const DirectX::XMVECTOR signVector = DirectX::XMVectorDivide(newWrapVector, DirectX::XMVectorAbs(newWrapVector));

		newWrapVector = DirectX::XMVectorSubtract(DirectX::XMVectorReplicate(1.0f), DirectX::XMVectorAbs(DirectX::XMVectorSwizzle<DirectX::XM_SWIZZLE_Y, DirectX::XM_SWIZZLE_X, DirectX::XM_SWIZZLE_Z, DirectX::XM_SWIZZLE_W>(newWrapVector)));

		// signVector should already have 0's in its z and w components, so just multiplying
		// wrapVector by it should cancel these components out again.
		return DirectX::XMVectorMultiply(newWrapVector, signVector);
	}

	DirectX::XMVECTOR XM_CALLCONV OctahedronEncodeNormal(DirectX::FXMVECTOR normal)
	{
		const float absComponentSum = DirectX::XMVectorGetX(DirectX::XMVectorSum(DirectX::XMVectorAbs(normal)));
		DirectX::XMVECTOR newNormal = DirectX::XMVectorDivide(normal, DirectX::XMVectorReplicate(absComponentSum));

		newNormal = OctahedronWrap(newNormal);

		return DirectX::XMVectorAdd(DirectX::XMVectorMultiply(newNormal, DirectX::XMVectorReplicate(0.5f)), DirectX::XMVectorSet(0.5f, 0.5f, 0.0f, 0.0f));
	}

	TangentFrame XM_CALLCONV CreateTangentFrame(DirectX::FXMVECTOR normal, DirectX::FXMVECTOR tangent)
	{
		DirectX::XMVECTOR tangent_b{};

		if (std::abs(DirectX::XMVectorGetX(normal)) > std::abs(DirectX::XMVectorGetZ(normal)))
			tangent_b = DirectX::XMVectorMultiply(DirectX::XMVectorSwizzle<DirectX::XM_SWIZZLE_Y, DirectX::XM_SWIZZLE_X, DirectX::XM_SWIZZLE_Z, DirectX::XM_SWIZZLE_W>(normal), DirectX::XMVectorSet(-1.0f, 1.0f, 0.0f, 0.0f));
		else
			tangent_b = DirectX::XMVectorMultiply(DirectX::XMVectorSwizzle<DirectX::XM_SWIZZLE_X, DirectX::XM_SWIZZLE_Z, DirectX::XM_SWIZZLE_Y, DirectX::XM_SWIZZLE_W>(normal), DirectX::XMVectorSet(0.0f, -1.0f, 1.0f, 0.0f));

		// Find the angle between tangent and tangent_b. This may or may not be the same angle
		// as the angle we will need to rotate tangent_b about the normal to get the tangent in
		// shaders.
		const float angleBetween = DirectX::XMVectorGetX(DirectX::XMVectorACos(DirectX::XMVector3Dot(tangent, tangent_b)));

		// Verify that we have the right angle by rotating tangent_b about the normal and seeing
		// if we get tangent back. We don't need to use a quaternion for this, just Rodrigues'
		// formula.
		const DirectX::XMVECTOR calculatedTangent{ DirectX::XMVectorMultiply(tangent_b, DirectX::XMVectorReplicate(std::cos(angleBetween))) };

		// Check the angle between the calculated tangent vector and the original tangent vector.
		// If it is approximately 0 radians, then we have the right rotation angle; otherwise, we
		// need to rotate tangent_b in the negative/opposite direction. We can simulate this by instead
		// rotating it by -angleBetween radians in the positive direction (i.e., angleBetween radians
		// in the negative direction).
		static constexpr float ANGLE_EPSILON = 0.001f;
		const float rotationAngle = (DirectX::XMVectorGetX(DirectX::XMVectorACos(DirectX::XMVector3Dot(tangent, calculatedTangent))) <= ANGLE_EPSILON ? angleBetween : -angleBetween);

		TangentFrame tangentFrame{
			.OctahedronEncodedNormal{},
			.RotationAngle = rotationAngle
		};
		DirectX::XMStoreFloat2(&(tangentFrame.OctahedronEncodedNormal), OctahedronEncodeNormal(normal));

		return tangentFrame;
	}
}

namespace Brawler
{
	StaticVertexBuffer::StaticVertexBuffer(const aiMesh& mesh) :
		mUnpackedVertices(),
		mPackedVertices(),
		mBoundingBox(DirectX::XMFLOAT3{ AABB_MINIMUM_POINT_INIT }, DirectX::XMFLOAT3{ AABB_MAXIMUM_POINT_INIT })
	{
		// We will only support 16-bit indices... well, at least for now, anyways.
		const std::size_t vertexCount = static_cast<std::size_t>(mesh.mNumVertices);

		if (vertexCount > std::numeric_limits<std::uint16_t>::max()) [[unlikely]]
			throw std::runtime_error{ std::format("ERROR: The mesh {} has more vertices than can be represented with 16-bit indices (i.e., it has more than 65,536 vertices)!", mesh.mName.C_Str()) };

		if (vertexCount == 0) [[unlikely]]
			throw std::runtime_error{ std::format("ERROR: The mesh {} has no vertices!", mesh.mName.C_Str()) };

		mUnpackedVertices.reserve(vertexCount);
		mPackedVertices.reserve(vertexCount);

		InitializeUnpackedData(mesh);
	}

	void StaticVertexBuffer::Update()
	{
		// TODO: Create the equivalent GPU implementation. It should run much faster than
		// a CPU version. For now, however, during the first update, we can just initialize
		// all of the packed data.
		//
		// TODO 2: Do we actually need a GPU implementation? We seem to be bottlenecked on
		// the GPU already by texture compression.

		if (mPackedVertices.empty()) [[unlikely]]
			InitializePackedData();
	}

	bool StaticVertexBuffer::IsReadyForSerialization() const
	{
		// TODO: Create the equivalent GPU implementation. It should run much faster than
		// a CPU version. However, since we know that all of the data was prepared by the
		// time this function gets called, we can go ahead and return true.
		//
		// TODO 2: Do we actually need a GPU implementation? We seem to be bottlenecked on
		// the GPU already by texture compression.

		return true;
	}

	void StaticVertexBuffer::InitializePackedData()
	{
		for (const auto& unpackedVertex : mUnpackedVertices)
		{
			// TODO: Create the equivalent GPU implementation. It should run much faster than
			// a CPU version.
			//
			// TODO 2: Do we actually need a GPU implementation? We seem to be bottlenecked on
			// the GPU already by texture compression.

			const TangentFrame tangentFrame{ CreateTangentFrame(DirectX::XMLoadFloat3(&(unpackedVertex.Normal)), DirectX::XMLoadFloat3(&(unpackedVertex.Tangent))) };

			// We'll need to do some pretty ugly casting to pack this TangentFrame into a
			// float... I'm terribly sorry for this, my fellow C++ comrades.
			std::uint32_t packedTangentFrame = 0;

			{
				// Convert the x-component of the encoded normal from [0, 1] to [0, 255].
				const std::uint32_t compressedFrameX = static_cast<std::uint32_t>(tangentFrame.OctahedronEncodedNormal.x * 255.0f);

				packedTangentFrame |= compressedFrameX;
			}

			{
				// Convert the y-component of the encoded normal from [0, 1] to [0, 255].
				const std::uint32_t compressedFrameY = (static_cast<std::uint32_t>(tangentFrame.OctahedronEncodedNormal.y * 255.0f) << 8);

				packedTangentFrame |= compressedFrameY;
			}

			{
				// Convert the rotation angle from [0, (2 * PI)] to [0, 255].
				static constexpr float CONVERSION_FACTOR = (255.0f / DirectX::XM_2PI);
				const std::uint32_t compressedRotationAngle = (static_cast<std::uint32_t>(tangentFrame.RotationAngle * CONVERSION_FACTOR) << 16);

				packedTangentFrame |= compressedRotationAngle;
			}

			mPackedVertices.push_back(PackedStaticVertex{
				.PositionAndTangentFrame{ unpackedVertex.Position.x, unpackedVertex.Position.y, unpackedVertex.Position.z, std::bit_cast<float>(packedTangentFrame)},
				.UVCoords{ unpackedVertex.UVCoords }
			});
		}
	}

	void StaticVertexBuffer::InitializeUnpackedData(const aiMesh& mesh)
	{
		if (!mesh.HasNormals()) [[unlikely]]
			throw std::runtime_error{ std::string{ "ERROR: The mesh " } + mesh.mName.C_Str() + " does not have any normals!" };
		
		for (std::size_t i = 0; i < static_cast<std::size_t>(mesh.mNumVertices); ++i)
		{
			UnpackedStaticVertex unpackedVertex{
				.Position{ mesh.mVertices[i].x, mesh.mVertices[i].y, mesh.mVertices[i].z },
				.Normal{ mesh.mNormals[i].x, mesh.mNormals[i].y, mesh.mNormals[i].z },
				.Tangent{ mesh.mTangents[i].x, mesh.mTangents[i].y, mesh.mTangents[i].z },
				.UVCoords{ mesh.mTextureCoords[0][i].x, mesh.mTextureCoords[0][i].y }
			};

			// Adjust the AABB to include this vertex.
			mBoundingBox.InsertPoint(DirectX::XMLoadFloat3(&(unpackedVertex.Position)));

			mUnpackedVertices.push_back(std::move(unpackedVertex));
		}
	}
}