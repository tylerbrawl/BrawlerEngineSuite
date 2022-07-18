module;
#include <cstdint>

export module Brawler.IndexBufferHeader;

export namespace Brawler
{
	struct IndexBufferHeader
	{
		std::uint32_t TriangleClusterCount;
		std::uint32_t NumTrianglesInFinalCluster;
	};

#pragma pack(push)
#pragma pack(1)
	struct SerializedIndexBufferHeader
	{
		std::uint32_t TriangleClusterCount;
		std::uint32_t NumTrianglesInFinalCluster;
	};
#pragma pack(pop)
}