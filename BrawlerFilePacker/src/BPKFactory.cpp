module;
#include <vector>
#include <memory>
#include <string>
#include <array>
#include <span>
#include <cassert>
#include <filesystem>
#include <stdexcept>
#include <fstream>
#include <unordered_map>

module Brawler.BPKFactory;
import Brawler.BCAArchive;
import Brawler.AssetCompilerContext;
import Brawler.BCAMetadata;
import Brawler.PackerSettings;
import Brawler.ZSTDFrame;

namespace
{
	static constexpr std::string_view BPK_MAGIC = "BPK";

	struct CommonBPKFileHeader
	{
		std::array<char, (BPK_MAGIC.size() + 1)> Magic;
		std::uint32_t Version;
	};

	std::ofstream& operator<<(std::ofstream& lhs, const CommonBPKFileHeader& rhs)
	{
		lhs.write(rhs.Magic.data(), rhs.Magic.size());
		lhs.write(reinterpret_cast<const char*>(&(rhs.Version)), sizeof(rhs.Version));

		return lhs;
	}

	struct TableOfContentsEntryV1
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

	std::ofstream& operator<<(std::ofstream& lhs, const TableOfContentsEntryV1& rhs)
	{
		lhs.write(reinterpret_cast<const char*>(&(rhs.FileIdentifierHash)), sizeof(rhs.FileIdentifierHash));
		lhs.write(reinterpret_cast<const char*>(&(rhs.FileOffsetInBytes)), sizeof(rhs.FileOffsetInBytes));
		lhs.write(reinterpret_cast<const char*>(&(rhs.CompressedSizeInBytes)), sizeof(rhs.CompressedSizeInBytes));
		lhs.write(reinterpret_cast<const char*>(&(rhs.UncompressedSizeInBytes)), sizeof(rhs.UncompressedSizeInBytes));

		return lhs;
	}

	struct VersionedBPKFileHeaderV1
	{
		/// <summary>
		/// This is the size, in bytes, of the entire table of contents (ToC) for this
		/// BPK file.
		/// </summary>
		std::size_t TableOfContentsSizeInBytes;

		/// <summary>
		/// This is a type alias for the struct representing an entry in the ToC. Every
		/// versioned BPK file header must provide a type alias for this, although they
		/// do not necessarily need to have different type values (i.e., a ToC entry
		/// struct type can be re-used between versioned BPK file headers).
		/// </summary>
		using TableOfContentsEntry = TableOfContentsEntryV1;
	};

	std::ofstream& operator<<(std::ofstream& lhs, const VersionedBPKFileHeaderV1& rhs)
	{
		lhs.write(reinterpret_cast<const char*>(&(rhs.TableOfContentsSizeInBytes)), sizeof(rhs.TableOfContentsSizeInBytes));

		return lhs;
	}

	using CurrentVersionedBPKFileHeader = VersionedBPKFileHeaderV1;
}

namespace Brawler
{
	template <>
	VersionedBPKFileHeaderV1 BPKFactory::CreateVersionedBPKFileHeader() const
	{
		static constexpr std::size_t TOC_ENTRY_SIZE = sizeof(VersionedBPKFileHeaderV1::TableOfContentsEntry);
		
		return VersionedBPKFileHeaderV1{
			.TableOfContentsSizeInBytes{TOC_ENTRY_SIZE * mBCAArchiveArr.size()}
		};
	}

	template <>
	void BPKFactory::WriteTableOfContents<VersionedBPKFileHeaderV1>(std::ofstream& bpkFileStream) const
	{
		const std::size_t totalTOCSize{ sizeof(VersionedBPKFileHeaderV1::TableOfContentsEntry) * mBCAArchiveArr.size() };

		// Create and write out a ToC entry for every file which will be placed into the
		// BPK archive.
		// 
		// TODO: Should we add padding for alignment? If so, how much?
		std::uint64_t currFileOffset = sizeof(CommonBPKFileHeader) + sizeof(VersionedBPKFileHeaderV1) + totalTOCSize;

		for (const auto& bcaArchivePtr : mBCAArchiveArr)
		{
			VersionedBPKFileHeaderV1::TableOfContentsEntry tocEntry{
				.FileIdentifierHash{bcaArchivePtr->GetMetadata().SourceAssetDirectoryHash},
				.FileOffsetInBytes{currFileOffset},
				.CompressedSizeInBytes{bcaArchivePtr->GetCompressedAssetFrame().GetByteArray().size_bytes()},
				.UncompressedSizeInBytes{bcaArchivePtr->GetMetadata().UncompressedSizeInBytes}
			};
			bpkFileStream << tocEntry;

			// TODO: Should we add padding for alignment? If so, how much?
			currFileOffset += tocEntry.CompressedSizeInBytes;
		}
	}

	BPKFactory::BPKFactory(std::vector<std::unique_ptr<BCAArchive>>&& bcaArchiveArr) :
		mBCAArchiveArr(std::move(bcaArchiveArr))
	{}

	void BPKFactory::CreateBPKArchive(const AssetCompilerContext& context) const
	{
		std::filesystem::path bpkOutputPath{ context.RootOutputDirectory / L"Compiled Packages" / L"Data.bpk" };

		WriteBPKFile(bpkOutputPath);
	}

	void BPKFactory::WriteBPKFile(const std::filesystem::path& bpkOutputPath) const
	{
		std::ofstream bpkFileStream{ bpkOutputPath, std::ios_base::out | std::ios_base::binary };

		// Write out the common BPK file header.
		{
			CommonBPKFileHeader commonHeader{
				.Magic{},
				.Version{ PackerSettings::TARGET_BPK_VERSION }
			};
			std::memcpy(commonHeader.Magic.data(), BPK_MAGIC.data(), BPK_MAGIC.size());

			bpkFileStream << commonHeader;
		}

		// Write out the current versioned BPK file header.
		{
			CurrentVersionedBPKFileHeader versionedHeader{ CreateVersionedBPKFileHeader<CurrentVersionedBPKFileHeader>() };
			bpkFileStream << versionedHeader;
		}

		// Write out the Table of Contents (ToC).
		WriteTableOfContents<CurrentVersionedBPKFileHeader>(bpkFileStream);

		// Write out the compressed file archives.
		for (const auto& bcaArchivePtr : mBCAArchiveArr)
		{
			// TODO: Should we add padding for alignment? If so, how much?
			bpkFileStream << bcaArchivePtr->GetCompressedAssetFrame();
		}
	}
}