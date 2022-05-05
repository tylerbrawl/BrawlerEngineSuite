module;
#include <unordered_map>
#include <fstream>

export module Brawler.BPKArchiveReader;
import Brawler.FileMapper;
import Brawler.MappedFileView;

export namespace Brawler
{
	class FilePathHash;
}

export namespace Brawler
{
	class BPKArchiveReader
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
			/// </summary>
			std::uint64_t CompressedSizeInBytes;

			/// <summary>
			/// This is the size, in bytes, of the uncompressed data represented by this
			/// ToC entry.
			/// </summary>
			std::uint64_t UncompressedSizeInBytes;
		};

	public:
		BPKArchiveReader();

		BPKArchiveReader(const BPKArchiveReader& rhs) = delete;
		BPKArchiveReader& operator=(const BPKArchiveReader& rhs) = delete;

		BPKArchiveReader(BPKArchiveReader&& rhs) noexcept = default;
		BPKArchiveReader& operator=(BPKArchiveReader&& rhs) noexcept = default;

		const TOCEntry& GetTableOfContentsEntry(const FilePathHash& pathHash) const;

		FileMapper& GetBPKFileMapper();
		const FileMapper& GetBPKFileMapper() const;

	private:
		/// <summary>
		/// This is a map between a file path hash and the remainder of its respective
		/// Table of Contents (ToC) entry in a BPK file.
		/// </summary>
		const std::unordered_map<std::uint64_t, TOCEntry> mTableOfContents;

		FileMapper mBPKMapper;
	};
}