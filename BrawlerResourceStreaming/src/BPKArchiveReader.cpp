module;
#include <string>
#include <unordered_map>
#include <array>
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <cassert>

module Brawler.BPKArchiveReader;
import Brawler.FilePathHash;
import Brawler.FileAccessMode;

namespace
{
	static constexpr std::wstring_view DATA_SUBDIRECTORY = L"Data\\Data.bpk";
	static constexpr std::string_view BPK_MAGIC = "BPK";
	static constexpr std::uint32_t CURRENT_BPK_VERSION = 1;

	struct CommonBPKFileHeader
	{
		std::array<char, (BPK_MAGIC.size() + 1)> Magic;
		std::uint32_t Version;
	};

	std::ifstream& operator>>(std::ifstream& lhs, CommonBPKFileHeader& rhs)
	{
		lhs.read(reinterpret_cast<char*>(rhs.Magic.data()), rhs.Magic.size());
		lhs.read(reinterpret_cast<char*>(&(rhs.Version)), sizeof(rhs.Version));

		return lhs;
	}

	struct CurrentVersionedBPKFileHeader
	{
		/// <summary>
		/// This is the size, in bytes, of the entire table of contents (ToC) for this
		/// BPK file.
		/// </summary>
		std::size_t TableOfContentsSizeInBytes;
	};

	std::ifstream& operator>>(std::ifstream& lhs, CurrentVersionedBPKFileHeader& rhs)
	{
		lhs.read(reinterpret_cast<char*>(&(rhs.TableOfContentsSizeInBytes)), sizeof(rhs.TableOfContentsSizeInBytes));

		return lhs;
	}

	struct BPKTableOfContentsEntry
	{
		/// <summary>
		/// This is the hash used to uniquely identify the file. The least significant
		/// byte of this hash should be the same as the name of the BPK file which it
		/// is located in.
		/// </summary>
		std::uint64_t FileIdentifierHash;

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

	std::ifstream& operator>>(std::ifstream& lhs, BPKTableOfContentsEntry& rhs)
	{
		lhs.read(reinterpret_cast<char*>(&(rhs.FileIdentifierHash)), sizeof(rhs.FileIdentifierHash));
		lhs.read(reinterpret_cast<char*>(&(rhs.FileOffsetInBytes)), sizeof(rhs.FileOffsetInBytes));
		lhs.read(reinterpret_cast<char*>(&(rhs.CompressedSizeInBytes)), sizeof(rhs.CompressedSizeInBytes));
		lhs.read(reinterpret_cast<char*>(&(rhs.UncompressedSizeInBytes)), sizeof(rhs.UncompressedSizeInBytes));

		return lhs;
	}

	static const std::filesystem::path BPK_ARCHIVE_PATH = [] ()
	{
		std::filesystem::path bpkPath{ std::filesystem::current_path() / std::filesystem::path{ DATA_SUBDIRECTORY } };

		// Make sure that the data archive exists.
		if (!std::filesystem::exists(bpkPath))
			throw std::runtime_error{ "ERROR: The data required for this application cannot be found." };

		return bpkPath;
	}();

	/// <summary>
	/// Ensures that the application's BPK archive is valid and attempts to the current
	/// versioned BPK file header.
	/// </summary>
	/// <returns>
	/// If the BPK file is valid, then the std::optional instance returned by this function
	/// contains its versioned BPK file header. Otherwise, the returned std::optional instance
	/// is empty.
	/// </returns>
	std::optional<CurrentVersionedBPKFileHeader> TryExtractVersionedBPKFileHeader(std::ifstream& bpkFileStream)
	{
		// Read the common BPK file header.
		{
			CommonBPKFileHeader commonHeader{};
			bpkFileStream >> commonHeader;

			for (std::size_t i = 0; i < BPK_MAGIC.size(); ++i)
			{
				if (commonHeader.Magic[i] != BPK_MAGIC[i]) [[unlikely]]
					return std::optional<CurrentVersionedBPKFileHeader>{};
			}

			if (commonHeader.Version != CURRENT_BPK_VERSION) [[unlikely]]
				return std::optional<CurrentVersionedBPKFileHeader>{};
		}

		// Read the current versioned BPK file header.
		{
			CurrentVersionedBPKFileHeader versionedHeader{};
			bpkFileStream >> versionedHeader;

			return std::optional<CurrentVersionedBPKFileHeader>{ std::move(versionedHeader) };
		}
	}

	std::unordered_map<std::uint64_t, Brawler::BPKArchiveReader::TOCEntry> CreateTableOfContents()
	{
		std::ifstream bpkFileStream{ BPK_ARCHIVE_PATH, std::ios_base::in | std::ios_base::binary };
		std::unordered_map<std::uint64_t, Brawler::BPKArchiveReader::TOCEntry> tableOfContents{};
		std::optional<CurrentVersionedBPKFileHeader> versionedHeader{ TryExtractVersionedBPKFileHeader(bpkFileStream) };

		if (!versionedHeader.has_value()) [[unlikely]]
			throw std::runtime_error{ "ERROR: The versioned BPK file header could not be extracted from the application's BPK archive!" };

		const std::size_t numTOCEntries = versionedHeader->TableOfContentsSizeInBytes / sizeof(BPKTableOfContentsEntry);

		for (std::size_t i = 0; i < numTOCEntries; ++i)
		{
			BPKTableOfContentsEntry rawTOCEntry{};
			bpkFileStream >> rawTOCEntry;

			tableOfContents.try_emplace(
				rawTOCEntry.FileIdentifierHash,
				rawTOCEntry.FileOffsetInBytes,
				rawTOCEntry.CompressedSizeInBytes,
				rawTOCEntry.UncompressedSizeInBytes
			);
		}

		return tableOfContents;
	}
}

namespace Brawler
{
	BPKArchiveReader::BPKArchiveReader() :
		mTableOfContents(CreateTableOfContents()),
		mBPKMapper(std::filesystem::path{ std::filesystem::current_path() / DATA_SUBDIRECTORY }, FileAccessMode::READ_ONLY)
	{}

	const BPKArchiveReader::TOCEntry& BPKArchiveReader::GetTableOfContentsEntry(const FilePathHash& pathHash) const
	{
		assert(mTableOfContents.contains(pathHash.GetHash()) && "ERROR: An attempt was made to get the BPK Table of Contents entry for a file which does not exist within the archive!");
		return mTableOfContents.at(pathHash.GetHash());
	}

	FileMapper& BPKArchiveReader::GetBPKFileMapper()
	{
		return mBPKMapper;
	}

	const FileMapper& BPKArchiveReader::GetBPKFileMapper() const
	{
		return mBPKMapper;
	}
}