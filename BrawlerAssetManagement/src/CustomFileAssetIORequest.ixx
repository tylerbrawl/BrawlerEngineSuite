module;
#include <filesystem>

export module Brawler.AssetManagement.CustomFileAssetIORequest;

export namespace Brawler
{
	namespace AssetManagement
	{
		struct CustomFileAssetIORequest
		{
			/// <summary>
			/// Specifies the path to the file which is to be loaded. This does not have to be
			/// the BPK archive, but it can be.
			/// 
			/// To get the path of the BPK archive, call BPKArchiveReader::GetBPKArchiveFilePath().
			/// </summary>
			const std::filesystem::path& FilePath;

			/// <summary>
			/// The offset, in bytes, from the start of the file to begin reading data.
			/// </summary>
			std::size_t FileOffset;

			/// <summary>
			/// The size, in bytes, of the compressed data which must be read. If this value is 0,
			/// then the data is assumed to *NOT* be compressed; otherwise, zstandard is used to
			/// decompress the data.
			/// </summary>
			std::size_t CompressedDataSizeInBytes;

			/// <summary>
			/// The size, in bytess, of the uncompressed data which must be read. The destination
			/// for the request's data *MUST* be large enough to hold this amount of bytes.
			/// </summary>
			std::size_t UncompressedDataSizeInBytes;
		};
	}
}