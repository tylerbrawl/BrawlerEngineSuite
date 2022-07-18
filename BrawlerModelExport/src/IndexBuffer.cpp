module;
#include <vector>
#include <span>
#include <variant>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <assimp/mesh.h>

module Brawler.IndexBuffer;
import Util.ModelExport;
import Util.General;
import Brawler.LaunchParams;
import Brawler.Math.AABB;

namespace Brawler
{
	IndexBuffer::IndexBuffer(const ImportedMesh& mesh) :
		mClusterArr(),
		mMeshPtr(&mesh)
	{
		const aiMesh& assimpMesh{ mesh.GetMesh() };

		// We should have guaranteed that the mesh was triangulated in the SceneLoader class.
		assert(assimpMesh.mPrimitiveTypes == aiPrimitiveType::aiPrimitiveType_TRIANGLE && "ERROR: A mesh was not properly triangulated during loading!");

		static constexpr std::size_t MAXIMUM_TRIANGLES_PER_CLUSTER = TriangleCluster::GetMaximumTriangleCount();

		TriangleCluster currCluster{ mesh };
		const std::span<const aiFace> meshFaceArr{ assimpMesh.mFaces, static_cast<std::size_t>(assimpMesh.mNumFaces) };

		for (const auto& face : meshFaceArr)
		{
			currCluster.AddTriangle(face);

			if (currCluster.GetTriangleCount() == MAXIMUM_TRIANGLES_PER_CLUSTER) [[unlikely]]
			{
				mClusterArr.push_back(std::move(currCluster));
				currCluster = TriangleCluster{ mesh };
			}
		}

		if (currCluster.HasTriangles()) [[likely]]
			mClusterArr.push_back(std::move(currCluster));
	}

	void IndexBuffer::Update()
	{}

	bool IndexBuffer::IsReadyForSerialization() const
	{
		return true;
	}

	FilePathHash IndexBuffer::SerializeIndexBuffer() const
	{
		assert(IsReadyForSerialization());
		assert(mMeshPtr != nullptr);

		const Brawler::LaunchParams& launchParams{ Util::ModelExport::GetLaunchParameters() };

		assert(mMeshPtr->GetLODScene().GetLODLevel() == 0);

		const std::filesystem::path outputFileSubDirectory{ L"Models" / std::filesystem::path{ launchParams.GetModelName() } / std::format(L"LOD{}_{}_IndexBuffer.ib", mMeshPtr->GetLODScene().GetLODLevel(), mMeshPtr->GetMeshIDForLOD()) };
		const FilePathHash indexBufferPathHash{ outputFileSubDirectory.c_str() };

		const std::filesystem::path fullOutputPath{ launchParams.GetRootOutputDirectory() / outputFileSubDirectory };
		std::error_code errorCode{};

		std::filesystem::create_directories(fullOutputPath.parent_path(), errorCode);
		Util::General::CheckErrorCode(errorCode);

		{
			std::ofstream indexBufferFileStream{ fullOutputPath, std::ios::out | std::ios::binary };
			
			// For each triangle cluster, write out its list of indices, followed by the minimum and
			// maximum points of its AABB.
			for (const auto& cluster : mClusterArr)
			{
				const std::span<const std::uint32_t> indexSpan{ cluster.GetIndexSpan() };
				indexBufferFileStream.write(reinterpret_cast<const char*>(indexSpan.data()), indexSpan.size_bytes());

				const Math::AABB clusterBoundingBox{ cluster.CalculateBoundingBox() };
				
				const DirectX::XMFLOAT3& aabbMinPoint{ clusterBoundingBox.GetMinimumBoundingPoint() };
				indexBufferFileStream.write(reinterpret_cast<const char*>(&aabbMinPoint), sizeof(aabbMinPoint));

				const DirectX::XMFLOAT3& aabbMaxPoint{ clusterBoundingBox.GetMaximumBoundingPoint() };
				indexBufferFileStream.write(reinterpret_cast<const char*>(&aabbMaxPoint), sizeof(aabbMaxPoint));
			}
		}

		return indexBufferPathHash;
	}

	IndexBufferHeader IndexBuffer::GetIndexBufferHeader() const
	{
		return IndexBufferHeader{
			.TriangleClusterCount = static_cast<std::uint32_t>(mClusterArr.size()),
			.NumTrianglesInFinalCluster = (mClusterArr.empty() ? 0 : static_cast<std::uint32_t>(mClusterArr.back().GetTriangleCount()))
		};
	}
}