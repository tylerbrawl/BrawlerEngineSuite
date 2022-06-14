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

namespace Brawler
{
	IndexBuffer::IndexBuffer(const ImportedMesh& mesh) :
		mIndexArr(),
		mMeshPtr(&mesh)
	{
		const aiMesh& assimpMesh{ mesh.GetMesh() };

		// We should have guaranteed that the mesh was triangulated in the SceneLoader class.
		assert(assimpMesh.mPrimitiveTypes == aiPrimitiveType::aiPrimitiveType_TRIANGLE && "ERROR: A mesh was not properly triangulated during loading!");

		mIndexArr.reserve(static_cast<std::size_t>(assimpMesh.mNumFaces) * 3);

		const std::span<const aiFace> meshFaceArr{ assimpMesh.mFaces, static_cast<std::size_t>(assimpMesh.mNumFaces) };
		for (const auto& face : meshFaceArr)
		{
			mIndexArr.push_back(face.mIndices[0]);
			mIndexArr.push_back(face.mIndices[1]);
			mIndexArr.push_back(face.mIndices[2]);
		}
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
			const std::span<const std::uint32_t> indexSpan{ mIndexArr };

			indexBufferFileStream.write(reinterpret_cast<const char*>(indexSpan.data()), indexSpan.size_bytes());
		}

		return indexBufferPathHash;
	}

	std::size_t IndexBuffer::GetIndexCount() const
	{
		return mIndexArr.size();
	}
}