module;
#include <cassert>
#include <array>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <format>

module Brawler.BCAArchive;
import Brawler.AssetCompilerContext;
import Brawler.PackerSettings;
import Brawler.BCAMetadata;
import Util.Threading;
import Brawler.ThreadLocalResources;
import Brawler.SHA512Hasher;
import Brawler.SHA512Hash;
import Util.Engine;
import Brawler.ZSTDFrame;
import Brawler.ZSTDContext;
import Brawler.StringHasher;
import Util.Win32;
import Brawler.BCAInfoDatabase;

namespace
{
	static constexpr std::string_view BCA_MAGIC = "BCA";

	/// <summary>
	/// This is the layout of the common BCA file header. This header data is consistent across
	/// all BCA versions.
	/// </summary>
	struct CommonBCAFileHeader
	{
		/// <summary>
		/// The first three bytes of a valid BCA file will be BCA.
		/// </summary>
		std::array<char, (BCA_MAGIC.size() + 1)> Magic;

		/// <summary>
		/// The next four bytes represent the version number.
		/// </summary>
		std::uint32_t Version;
	};

	std::ifstream& operator>>(std::ifstream& lhs, CommonBCAFileHeader& rhs)
	{
		lhs.read(rhs.Magic.data(), rhs.Magic.size());
		lhs.read(reinterpret_cast<char*>(&(rhs.Version)), sizeof(rhs.Version));

		return lhs;
	}

	std::ofstream& operator<<(std::ofstream& lhs, const CommonBCAFileHeader& rhs)
	{
		lhs.write(rhs.Magic.data(), rhs.Magic.size());
		lhs.write(reinterpret_cast<const char*>(&(rhs.Version)), sizeof(rhs.Version));

		return lhs;
	}

	// What follows is a list of versioned BCA file headers. These will appear immediately after
	// the common BCA file header in all BCA versions, but its contents are likely to differ
	// between versions.

	/// <summary>
	/// Version Number: 1
	/// </summary>
	struct VersionedBCAFileHeaderV1
	{
		/// <summary>
		/// The first byte represents the PackerSettings::BuildMode which was used when
		/// creating the BCA file.
		/// 
		/// NOTE: To ensure backwards compatibility, we cannot use 
		/// std::underlying_type_t<PackerSettings::BuildMode> to identify the type of this
		/// member, since this type may theoretically change between versions.
		/// </summary>
		std::uint8_t BuildMode;
		
		/// <summary>
		/// The next 64 bytes are the SHA-512 hash of the data *before* compression.
		/// </summary>
		Brawler::SHA512Hash UncompressedDataHash;
	};

	std::ifstream& operator>>(std::ifstream& lhs, VersionedBCAFileHeaderV1& rhs)
	{
		lhs.read(reinterpret_cast<char*>(&(rhs.BuildMode)), sizeof(rhs.BuildMode));
		
		std::array<std::uint8_t, Util::Engine::SHA_512_HASH_SIZE_IN_BYTES> hashByteArr{};
		lhs.read(reinterpret_cast<char*>(hashByteArr.data()), hashByteArr.size());

		rhs.UncompressedDataHash = Brawler::SHA512Hash{ std::move(hashByteArr) };
		return lhs;
	}

	std::ofstream& operator<<(std::ofstream& lhs, const VersionedBCAFileHeaderV1& rhs)
	{
		lhs.write(reinterpret_cast<const char*>(&(rhs.BuildMode)), sizeof(rhs.BuildMode));
		lhs.write(reinterpret_cast<const char*>(rhs.UncompressedDataHash.GetByteArray().data()), rhs.UncompressedDataHash.GetByteArray().size_bytes());

		return lhs;
	}

	using CurrentVersionedBCAFileHeader = VersionedBCAFileHeaderV1;
}

namespace Brawler
{
	// C++ Tip of the Day: Define explicit template specializations of member functions as early
	// as possible to avoid error C2908.

	template <>
	void BCAArchive::TryInitializeBCAArchiveFromFile<VersionedBCAFileHeaderV1>(std::ifstream& bcaFileStream)
	{
		VersionedBCAFileHeaderV1 versionedBCAHeader{};
		bcaFileStream >> versionedBCAHeader;

		// It may happen that there is a build mode mismatch between the data in the existing
		// BCA file and the currently chosen setting. This is probably unintentional, and we
		// should throw an exception in this case.
		if (static_cast<PackerSettings::BuildMode>(versionedBCAHeader.BuildMode) != Util::Engine::GetAssetBuildMode()) [[unlikely]]
			throw std::runtime_error{ "ERROR: There was a build mode mismatch between an existing BCA archive and the current build mode setting! (Did you set your command line arguments correctly?)" };

		SHA512Hash oldBCAHash{ std::move(versionedBCAHeader.UncompressedDataHash) };

		if (oldBCAHash != mMetadata.UncompressedDataHash)
			return;

		// We can just re-use this file instead of re-compressing the entire file again!
		mCompressedAssetFrame = ExtractCompressedAssetFromExistingBCAArchive<VersionedBCAFileHeaderV1>(bcaFileStream);
	}

	BCAArchive::BCAArchive(const AssetCompilerContext& context, std::filesystem::path&& assetDataPath) :
		mAssetDataPath(std::move(assetDataPath)),
		mBCAFilePath([&context] (const std::filesystem::path& assetPath)
		{
			// The idea is that we want the output directory for the .bca file to be similar
			// to the directory of the source asset. For example, if the output directory
			// is "D:\Brawler Engine\Data_Debug" and the source asset directory is
			// "D:\Brawler Engine\Source Assets\Audio\SFX\Gunshot01.wav," then we want the .bca file to
			// be found at "D:\Brawler Engine\Data_Debug\Asset Cache\Audio\SFX\Gunshot01.wav.bca."
			//
			// Due to some critical section shenanigans going on in the background, this is done manually
			// via std::string manipulation, rather than through std::filesystem::path manipulation. The
			// code is not as concise, but it runs *MUCH* better.

			const std::wstring assetCacheDir{ std::filesystem::path{ context.RootOutputDirectory / L"Asset Cache" }.wstring() };
			std::wstring assetDirStr{ assetPath.wstring() };
			assetDirStr.replace(0, context.RootDataDirectory.wstring().size(), assetCacheDir);

			std::filesystem::path assetBCAPath{ assetDirStr };
			const std::wstring bcaExtension{ assetBCAPath.extension().wstring() + L".bca" };
			assetBCAPath.replace_extension(bcaExtension);

			return assetBCAPath;
		}(mAssetDataPath)),
		mAssetDataBuffer(),
		mCompressedAssetFrame(),
		mMetadata(),
		mBCAInfoPtr(nullptr)
	{
		InitializeMetadata(context);
		InitializeBCAInfo();
	}

	void BCAArchive::InitializeArchiveData()
	{
		if (!mBCAInfoPtr->DoNotCompress) [[likely]]
			InitializeArchiveDataWithCompression();
		else [[unlikely]]
			InitializeArchiveDataWithoutCompression();

		// Free up the heap memory consumed for the original asset data, since
		// we do not need it anymore.
		mAssetDataBuffer.clear();
		mAssetDataBuffer.shrink_to_fit();
	}

	const std::filesystem::path& BCAArchive::GetAssetDataPath() const
	{
		return mAssetDataPath;
	}

	const BCAMetadata& BCAArchive::GetMetadata() const
	{
		return mMetadata;
	}

	const ZSTDFrame& BCAArchive::GetCompressedAssetFrame() const
	{
		return mCompressedAssetFrame;
	}

	const BCAInfo& BCAArchive::GetBCAInfo() const
	{
		assert(mBCAInfoPtr != nullptr);
		return *mBCAInfoPtr;
	}

	void BCAArchive::InitializeMetadata(const AssetCompilerContext& context)
	{
		mMetadata.BCAVersionNumber = PackerSettings::TARGET_BCA_VERSION;
		mMetadata.UncompressedSizeInBytes = std::filesystem::file_size(mAssetDataPath);

		// Get the hash of the relevant sub-directory of the source asset. Details
		// regarding what constitutes the "relevant" sub-directory are given in
		// the documentation for BCAMetadata::SourceAssetDirectoryHash.
		{
			std::wstring assetSubdirectoryStr{ mAssetDataPath.wstring() };
			assetSubdirectoryStr.erase(0, context.RootDataDirectory.wstring().size());

			// Erase any leading slashes (\).
			while (!assetSubdirectoryStr.empty() && assetSubdirectoryStr[0] == L'\\')
				assetSubdirectoryStr.erase(0, 1);

			StringHasher assetSubdirectoryHasher{ std::wstring_view{ assetSubdirectoryStr } };
			mMetadata.SourceAssetDirectoryHash = assetSubdirectoryHasher.GetHash();
		}

		// Cache the contents of the file. We store it as a member because we will need
		// it again for compression if we cannot re-use an existing file.
		mAssetDataBuffer.resize(mMetadata.UncompressedSizeInBytes);

		std::ifstream assetFileStream{ mAssetDataPath, std::ios_base::in | std::ios_base::binary };
		assetFileStream.read(reinterpret_cast<char*>(mAssetDataBuffer.data()), mAssetDataBuffer.size());

		mMetadata.UncompressedDataHash = Util::Threading::GetThreadLocalResources().Hasher.CreateSHA512Hash(mAssetDataBuffer);
	}

	void BCAArchive::InitializeBCAInfo()
	{
		// We delay retrieving the BCAInfo from the BCAInfoDatabase until we get to this
		// point because the data is generated by another thread concurrently, and there
		// are things which we can do without the BCAInfo, such as hashing the uncompressed
		// data.

		mBCAInfoPtr = std::addressof(BCAInfoDatabase::GetInstance().GetBCAInfoForSourceAsset(mAssetDataPath));
	}

	void BCAArchive::InitializeArchiveDataWithCompression()
	{
		// If it is possible, try to extract the compressed asset data from an
		// existing BCA file. If we succeed, then mCompressedAssetFrame will
		// no longer be empty.
		TryReUsePreCompiledAsset();

		// Create the BCA archive. Even if we are able to re-use compressed data,
		// we still want to make sure that our headers are up-to-date.
		CreateBCAArchive();

		// Report that the archive compilation was successful.
		Util::Win32::WriteFormattedConsoleMessage(mAssetDataPath.wstring() + L" -> " + mBCAFilePath.wstring());
	}

	void BCAArchive::InitializeArchiveDataWithoutCompression()
	{
		// If compression is disabled, then there is no point in creating or checking
		// an actual BCA file. This is because those files are meant to store compressed
		// asset data in order to improve build times for BPK archives.
		//
		// In this case, we only really need to move the uncompressed asset data into
		// the mCompressedAssetFrame field. The BCALinker will check if the file was
		// compressed or not.
		mCompressedAssetFrame = ZSTDFrame{ std::move(mAssetDataBuffer) };

		// Report that no archive was compiled because compression was disabled for
		// this asset.
		Util::Win32::WriteFormattedConsoleMessage(std::format(L"{} -> [Compression Disabled - No .bca File Generated]", mAssetDataPath.c_str()));
	}

	void BCAArchive::TryReUsePreCompiledAsset()
	{
		if (!std::filesystem::exists(mBCAFilePath)) [[unlikely]]
			return;

		assert(std::filesystem::is_regular_file(mBCAFilePath));

		std::ifstream existingBCAFile{ mBCAFilePath, std::ios_base::in | std::ios_base::binary };

		// First, check the common BCA file header.
		CommonBCAFileHeader commonHeader{};
		existingBCAFile >> commonHeader;

		// Ensure that the magic header matches.
		for (std::size_t i = 0; i < BCA_MAGIC.size(); ++i)
		{
			if (commonHeader.Magic[i] != BCA_MAGIC[i]) [[unlikely]]
				return;
		}

		// After the common BCA file header comes the corresponding versioned BCA
		// file header.
		switch (commonHeader.Version)
		{
		case PackerSettings::TARGET_BCA_VERSION: [[likely]]  // This is the current target version.
		{
			TryInitializeBCAArchiveFromFile<CurrentVersionedBCAFileHeader>(existingBCAFile);
			return;
		}

		// Add cases for different versions as they become outdated here.

		default: [[unlikely]]
			// We don't recognize this version, so we cannot use this file.
			return;
		}
	}

	template <typename VersionedBCAHeaderType>
	ZSTDFrame BCAArchive::ExtractCompressedAssetFromExistingBCAArchive(std::ifstream& bcaFileStream) const
	{
		// We don't immediately know the size of the compressed data. However, we
		// can calculate it as the file size minus the size of both the common and
		// versioned BCA header files.

		const std::size_t compressedFrameSize = std::filesystem::file_size(mBCAFilePath) - (sizeof(CommonBCAFileHeader) + sizeof(VersionedBCAHeaderType));
		std::vector<std::uint8_t> frameByteArr{};
		frameByteArr.resize(compressedFrameSize);

		bcaFileStream.read(reinterpret_cast<char*>(frameByteArr.data()), compressedFrameSize);

		return ZSTDFrame{ std::move(frameByteArr) };
	}

	void BCAArchive::CreateBCAArchive()
	{
		std::error_code parentDirCreationError{};
		std::filesystem::create_directories(mBCAFilePath.parent_path(), parentDirCreationError);

		if (parentDirCreationError) [[unlikely]]
			throw std::runtime_error{ std::string{"ERROR: The BCA output directory "} + mBCAFilePath.parent_path().string() + std::string{" could not be created for the following reason: "} + parentDirCreationError.message() };

		std::ofstream bcaFileStream{ mBCAFilePath, std::ios_base::out | std::ios_base::binary };
		
		// Write out the common BCA file header.
		{
			CommonBCAFileHeader commonHeader{
				.Magic{},
				.Version = PackerSettings::TARGET_BCA_VERSION
			};
			std::memcpy(commonHeader.Magic.data(), BCA_MAGIC.data(), BCA_MAGIC.size());

			bcaFileStream << commonHeader;
		}

		// Write out the versioned BCA file header.
		{
			static_assert(std::is_same_v<CurrentVersionedBCAFileHeader, VersionedBCAFileHeaderV1>, "ERROR: The definition for CurrentVersionedBCAFileHeader within BCAArchive::CreateBCAArchive() is outdated!");

			CurrentVersionedBCAFileHeader versionedBCAHeader{
				.BuildMode = std::to_underlying(Util::Engine::GetAssetBuildMode()),
				.UncompressedDataHash = mMetadata.UncompressedDataHash
			};

			bcaFileStream << versionedBCAHeader;
		}
		
		// Compress the asset data and write out the generated ZSTDFrame to the BCA file.
		// This can take a LONG time in Release builds. However, if mCompressedAssetFrame
		// is not empty, then we were able to get the data from a previous build; thus,
		// we do not need to compress the data again.
		{
			if (mCompressedAssetFrame.IsEmpty())
				mCompressedAssetFrame = ZSTDFrame{ Util::Threading::GetThreadLocalResources().ZSTDContext.CompressData(mAssetDataBuffer) };

			bcaFileStream << mCompressedAssetFrame;
		}
	}
}