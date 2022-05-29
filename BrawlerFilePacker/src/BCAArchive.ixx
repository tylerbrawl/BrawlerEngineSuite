module;
#include <filesystem>
#include <fstream>
#include <vector>

export module Brawler.BCAArchive;
import Brawler.BCAMetadata;
import Brawler.ZSTDFrame;
import Brawler.BCAInfo;

export namespace Brawler
{
	struct AssetCompilerContext;
}

namespace
{
	struct VersionedBCAFileHeaderV1;
}

export namespace Brawler
{
	class BCAArchive
	{
	public:
		explicit BCAArchive(const AssetCompilerContext& context, std::filesystem::path&& assetDataPath);

		void InitializeArchiveData();

		/// <summary>
		/// Use this function to retrieve the directory path for the *uncompressed*
		/// source asset.
		/// </summary>
		/// <returns>
		/// This function returns a const& to a std::filesystem::path representing
		/// the directory path for the *uncompressed* source asset.
		/// </returns>
		const std::filesystem::path& GetAssetDataPath() const;

		/// <summary>
		/// Use this function to retrieve the metadata for the BCA archive file.
		/// </summary>
		/// <returns>
		/// This function returns a const& to the metadata for the BCA archive file.
		/// </returns>
		const BCAMetadata& GetMetadata() const;

		/// <summary>
		/// Use this function to retrieve the ZSTDFrame which represents the compressed
		/// asset data.
		/// </summary>
		/// <returns>
		/// This function returns a const& to the ZSTDFrame which represents the compressed
		/// asset data.
		/// </returns>
		const ZSTDFrame& GetCompressedAssetFrame() const;

		/// <summary>
		/// Use this function to retrieve the BCAInfo instance for the BCA archive file.
		/// </summary>
		/// <returns>
		/// This function returns a const& to the BCAInfo instance for the BCA archive file.
		/// </returns>
		const BCAInfo& GetBCAInfo() const;

	private:
		void InitializeMetadata(const AssetCompilerContext& context);
		void InitializeBCAInfo();

		void InitializeArchiveDataWithCompression();
		void InitializeArchiveDataWithoutCompression();

		/// <summary>
		/// Attempts to re-use an existing BCA archive from a previous compilation of the
		/// given asset. If the SHA-512 hash has not changed since the asset was last
		/// compiled, then we do not need to re-compile it. This can significantly improve
		/// build times for Release mode.
		/// </summary>
		void TryReUsePreCompiledAsset();

		template <typename VersionedBCAHeaderType>
		void TryInitializeBCAArchiveFromFile(std::ifstream& bcaFileStream);

		/// <summary>
		/// Extracts the ZSTD frame representing the compressed asset from the .bca file
		/// identified by bcaFileStream. The file stream's cursor *MUST* be immediately before
		/// the frame data (i.e., immediately after the versioned BCA file header).
		/// </summary>
		/// <typeparam name="VersionedBCAHeaderType">
		/// - The type of the versioned BCA file header corresponding to the BCA archive file
		/// identified by bcaFileStream.
		/// </typeparam>
		/// <param name="bcaFileStream">
		/// - A std::ifstream reading from the BCA archive file from which the compressed asset 
		/// is to be extracted. The cursor of bcaFileStream *MUST* be immediately before the
		/// frame data (i.e., immediately after the versioned BCA file header).
		/// </param>
		/// <returns>
		/// The function returns a ZSTDFrame which contains the data representing the compressed
		/// asset.
		/// </returns>
		template <typename VersionedBCAHeaderType>
		ZSTDFrame ExtractCompressedAssetFromExistingBCAArchive(std::ifstream& bcaFileStream) const;

		void CreateBCAArchive();

	private:
		const std::filesystem::path mAssetDataPath;
		const std::filesystem::path mBCAFilePath;

		/// <summary>
		/// This represents the data of the asset *before* compression. Its contents
		/// are emptied as soon as they are no longer needed.
		/// </summary>
		std::vector<std::uint8_t> mAssetDataBuffer;

		/// <summary>
		/// This represents the data of the asset *after* compression.
		/// </summary>
		ZSTDFrame mCompressedAssetFrame;

		BCAMetadata mMetadata;
		const BCAInfo* mBCAInfoPtr;
	};
}