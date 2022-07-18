module;
#include <cstdint>

module Brawler.StaticLODMeshDefinition;
import Brawler.MappedFileView;

namespace Brawler
{
	StaticLODMeshDefinition::StaticLODMeshDefinition(const FilePathHash bmdlFileHash, const std::uint32_t lodMeshID) :
		mBMDLFileHash(bmdlFileHash),
		mLODMeshID(lodMeshID)
	{

	}
}