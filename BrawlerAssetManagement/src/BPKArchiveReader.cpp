module;
#include <string>
#include <unordered_map>
#include <array>
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <cassert>
#include <DxDef.h>

module Brawler.AssetManagement.BPKArchiveReader;
import Brawler.FilePathHash;
import Brawler.FileAccessMode;
import Brawler.SerializedStruct;
import Util.Reflection;

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
		/// This is the hash used to uniquely identify the file.
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

	static const std::filesystem::path bpkArchivePath = [] ()
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

	std::unordered_map<std::uint64_t, Brawler::AssetManagement::BPKArchiveReader::TOCEntry> CreateTableOfContents()
	{
		std::ifstream bpkFileStream{ bpkArchivePath, std::ios_base::in | std::ios_base::binary };
		std::unordered_map<std::uint64_t, Brawler::AssetManagement::BPKArchiveReader::TOCEntry> tableOfContents{};
		std::optional<CurrentVersionedBPKFileHeader> versionedHeader{ TryExtractVersionedBPKFileHeader(bpkFileStream) };

		if (!versionedHeader.has_value()) [[unlikely]]
			throw std::runtime_error{ "ERROR: The versioned BPK file header could not be extracted from the application's BPK archive!" };

		const std::size_t numTOCEntries = versionedHeader->TableOfContentsSizeInBytes / sizeof(BPKTableOfContentsEntry);

		std::vector<BPKTableOfContentsEntry> tocEntryArr{};

		struct TestStruct
		{
			std::int32_t A;
			std::int32_t B;
		};

		Brawler::SerializedStruct<TestStruct> testSerializedStruct{};

		if constexpr (Brawler::IsInherentlySerializable<BPKTableOfContentsEntry>)
		{
			// Rather than sequentially reading each individual ToC entry, we can just read the entire ToC at once.
			// This can be significantly faster.
			
			tocEntryArr.resize(numTOCEntries);
			bpkFileStream.read(reinterpret_cast<char*>(tocEntryArr.data()), versionedHeader->TableOfContentsSizeInBytes);
		}
		else
		{
			// To ensure correct deserialization, we need to first copy the data into serializable versions of
			// BPKTableOfContentsEntry.
			std::vector<Brawler::SerializedStruct<BPKTableOfContentsEntry>> serializedTOCEntryArr{};
			serializedTOCEntryArr.resize(numTOCEntries);

			bpkFileStream.read(reinterpret_cast<char*>(serializedTOCEntryArr.data()), versionedHeader->TableOfContentsSizeInBytes);

			// Now, we need to individually de-serialize each entry.
			tocEntryArr.reserve(numTOCEntries);

			for (const auto& serializedTOCEntry : serializedTOCEntryArr)
				tocEntryArr.push_back(Brawler::DeserializeData(serializedTOCEntry));
		}

		for (const auto& tocEntry : tocEntryArr)
			tableOfContents.try_emplace(
				tocEntry.FileIdentifierHash,
				tocEntry.FileOffsetInBytes,
				tocEntry.CompressedSizeInBytes,
				tocEntry.UncompressedSizeInBytes
			);

		/*
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
		*/

		return tableOfContents;
	}
}

namespace Brawler
{
	namespace AssetManagement
	{
		BPKArchiveReader::BPKArchiveReader() :
			mTableOfContents(CreateTableOfContents())
		{}
		
		BPKArchiveReader& BPKArchiveReader::GetInstance()
		{
			static BPKArchiveReader instance{};
			return instance;
		}

		const BPKArchiveReader::TOCEntry& BPKArchiveReader::GetTableOfContentsEntry(const FilePathHash pathHash) const
		{
			assert(mTableOfContents.contains(pathHash.GetHash()) && "ERROR: An attempt was made to get the BPK Table of Contents entry for a file which does not exist within the archive! (Did you remember to call BPKArchiveReader::Initialize()? This isn't done in the constructor because it can/should be done in parallel with other initialization operations.)");
			return mTableOfContents.at(pathHash.GetHash());
		}

		MappedFileView<FileAccessMode::READ_ONLY> BPKArchiveReader::CreateMappedFileViewForAsset(const FilePathHash pathHash) const
		{
			const TOCEntry& tocEntry{ GetTableOfContentsEntry(pathHash) };
			
			MappedFileView<FileAccessMode::READ_ONLY>  mappedView{ bpkArchivePath, MappedFileView<FileAccessMode::READ_ONLY>::ViewParams{
				.FileOffsetInBytes = tocEntry.FileOffsetInBytes,
				.ViewSizeInBytes = (tocEntry.IsDataCompressed() ? tocEntry.CompressedSizeInBytes : tocEntry.UncompressedSizeInBytes)
			} };
			assert(mappedView.IsValidView() && "ERROR: Something went wrong when creating a MappedFileView for an asset in a BPK file!");

			return mappedView;
		}

		const std::filesystem::path& BPKArchiveReader::GetBPKArchiveFilePath()
		{
			return bpkArchivePath;
		}
	}
}