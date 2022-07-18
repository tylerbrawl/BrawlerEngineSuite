module;
#include <cstdint>

export module Brawler.StaticLODMeshDefinition;
import Brawler.I_LODMeshDefinition;
import Brawler.FilePathHash;
import Brawler.SceneNode;

export namespace Brawler
{
	class StaticLODMeshDefinition final : public I_LODMeshDefinition
	{
	public:
		explicit StaticLODMeshDefinition(const FilePathHash bmdlFileHash, const std::uint32_t lodMeshID);

		StaticLODMeshDefinition(const StaticLODMeshDefinition& rhs) = delete;
		StaticLODMeshDefinition& operator=(const StaticLODMeshDefinition& rhs) = delete;

		StaticLODMeshDefinition(StaticLODMeshDefinition&& rhs) noexcept = default;
		StaticLODMeshDefinition& operator=(StaticLODMeshDefinition&& rhs) noexcept = default;

		void AssignLODMeshToSceneNode(SceneNode& node) override;

	private:
		FilePathHash mBMDLFileHash;
		std::uint32_t mLODMeshID;
	};
}