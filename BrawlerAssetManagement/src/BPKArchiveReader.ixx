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
				/// If the data is compressed, then this is the size, in bytes, of the compressed
				/// data represented by this ToC entry. Otherwise, if the data is *NOT* compressed,
				/// then this value is zero (0).
				/// 
				/// Although it is possible to check if data is compressed manually by using
				/// this field, it is strongly recommended that code instead checks for this
				/// by calling BPKArchiveReader::TOCEntry::IsDataCompressed(). This will prevent
				/// code from breaking if the specification ever changes.
				/// </summary>
				std::uint64_t CompressedSizeInBytes;

				/// <summary>
				/// This is the size, in bytes, of the uncompressed data represented by this
				/// ToC entry.
				/// </summary>
				std::uint64_t UncompressedSizeInBytes;

				/// <summary>
				/// Determines whether or not the data represented by this ToC entry contained within
				/// the BPK archive is compressed.
				/// </summary>
				/// <returns>
				/// The function returns true if the associated data is compressed within the BPK
				/// archive and false otherwise.
				/// </returns>
				__forceinline constexpr bool IsDataCompressed() const;
			};

		private:
			BPKArchiveReader();

		public:
			virtual ~BPKArchiveReader() = default;

			BPKArchiveReader(const BPKArchiveReader& rhs) = delete;
			BPKArchiveReader& operator=(const BPKArchiveReader& rhs) = delete;

			BPKArchiveReader(BPKArchiveReader&& rhs) noexcept = default;
			BPKArchiveReader& operator=(BPKArchiveReader&& rhs) noexcept = default;

			static BPKArchiveReader& GetInstance();

			const TOCEntry& GetTableOfContentsEntry(const FilePathHash pathHash) const;
			MappedFileView<FileAccessMode::READ_ONLY> CreateMappedFileViewForAsset(const FilePathHash pathHash) const;

			static const std::filesystem::path& GetBPKArchiveFilePath();

		private:
			/// <summary>
			/// This is a map between a file path hash and the remainder of its respective
			/// Table of Contents (ToC) entry in a BPK file.
			/// </summary>
			std::unordered_map<std::uint64_t, TOCEntry> mTableOfContents;
		};
	}
}

// -------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace AssetManagement
	{
		__forceinline constexpr bool BPKArchiveReader::TOCEntry::IsDataCompressed() const
		{
			return (CompressedSizeInBytes != 0);
		}
	}
}