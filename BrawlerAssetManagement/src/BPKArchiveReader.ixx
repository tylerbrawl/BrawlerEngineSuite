module;
#include <unordered_map>
#include <filesystem>
#include <DxDef.h>

export module Brawler.AssetManagement.BPKArchiveReader;
import Brawler.MappedFileView;
import Brawler.FileAccessMode;
import Brawler.FilePathHash;

export namespace Brawler
{
	namespace AssetManagement
	{
		class BPKArchiveReader final
		{
		public:
			struct TOCEntry
			{
				/// <summary>
				/// This is the offset, in bytes, from the start of the BPK file to the start
				/// of the compressed data represented by this ToC entry.
				/// </summary>
				std::uint64_t FileOffsetInBytes;

				/// <summary>
				/// This is the size, in bytes, of the compressed data within the BPK file
				/// archive.
				/// 
				/// TODO: Make this value 0 if the data is uncompressed.
				/// </summary>
				std::uint64_t CompressedSizeInBytes;

				/// <summary>
				/// This is the size, in bytes, of the uncompressed data represented by this
				/// ToC entry.
				/// </summary>
				std::uint64_t UncompressedSizeInBytes;

				bool IsDataCompressed() const;
			};

		private:
			BPKArchiveReader() = default;

		public:
			virtual ~BPKArchiveReader() = default;

			BPKArchiveReader(const BPKArchiveReader& rhs) = delete;
			BPKArchiveReader& operator=(const BPKArchiveReader& rhs) = delete;

			BPKArchiveReader(BPKArchiveReader&& rhs) noexcept = default;
			BPKArchiveReader& operator=(BPKArchiveReader&& rhs) noexcept = default;

			static BPKArchiveReader& GetInstance();

			const TOCEntry& GetTableOfContentsEntry(const FilePathHash pathHash) const;
			MappedFileView<FileAccessMode::READ_ONLY> CreateMappedFileViewForAsset(const FilePathHash pathHash) const;

			const std::filesystem::path& GetBPKArchiveFilePath() const;

		private:
			/// <summary>
			/// This is a map between a file path hash and the remainder of its respective
			/// Table of Contents (ToC) entry in a BPK file.
			/// </summary>
			std::unordered_map<std::uint64_t, TOCEntry> mTableOfContents;

			HANDLE mHFileMappingObject;
		};
	}
}