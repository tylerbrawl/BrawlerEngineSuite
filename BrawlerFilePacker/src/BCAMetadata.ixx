module;
#include <cstdint>

export module Brawler.BCAMetadata;
import Brawler.SHA512Hash;

export namespace Brawler
{
	struct BCAMetadata
	{
		// This is the version number of the BCA archive.
		std::uint32_t BCAVersionNumber;

		// This is the size, in bytes, of the data *before* compression.
		std::uint64_t UncompressedSizeInBytes;

		// This is the hash taken from the appropriate sub-directory of the source
		// asset data. For instance, if the source asset were located in 
		// "D:\Brawler Engine\SourceAssets\Textures\NormalMap01.dds", then the string 
		// "Textures\NormalMap01.dds" would be hashed.
		std::uint64_t SourceAssetDirectoryHash;

		// This is the SHA-512 hash of the asset data *before* compression. It is used
		// to determine if the asset has changed since the last compression; this can
		// significantly speed-up project build times.
		SHA512Hash UncompressedDataHash;
	};
}