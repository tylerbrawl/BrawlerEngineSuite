module;
#include <vector>
#include <array>
#include <span>
#include <filesystem>
#include <unordered_map>
#include <DxDef.h>

export module Brawler.AssetManagement.DirectStorageAssetIORequestBuilder;
import Brawler.AssetManagement.I_AssetIORequestBuilder;
import Brawler.FilePathHash;
import Brawler.D3D12.I_BufferSubAllocation;
import Brawler.SerializedStruct;

export namespace Brawler
{
	namespace AssetManagement
	{
		class DirectStorageAssetIORequestBuilder final : public I_AssetIORequestBuilder
		{
		private:
			using DStorageRequestContainer = std::vector<DSTORAGE_REQUEST>;

		public:
			DirectStorageAssetIORequestBuilder(IDStorageFactory& dStorageFactory, IDStorageFile& bpkDStorageFile);

			DirectStorageAssetIORequestBuilder(const DirectStorageAssetIORequestBuilder& rhs) = delete;
			DirectStorageAssetIORequestBuilder& operator=(const DirectStorageAssetIORequestBuilder& rhs) = delete;

			DirectStorageAssetIORequestBuilder(DirectStorageAssetIORequestBuilder&& rhs) noexcept = default;
			DirectStorageAssetIORequestBuilder& operator=(DirectStorageAssetIORequestBuilder&& rhs) noexcept = default;

			void AddAssetIORequest(const Brawler::FilePathHash pathHash, Brawler::D3D12::I_BufferSubAllocation& bufferSubAllocation) override;

			void AddAssetIORequest(const CustomFileAssetIORequest& customFileRequest) override;

			std::span<const DSTORAGE_REQUEST> GetDStorageRequestSpan(const Brawler::JobPriority priority) const;

		private:
			DSTORAGE_SOURCE CreateDStorageSourceForBPKAsset(const Brawler::FilePathHash pathHash) const;

			IDStorageFile& GetDStorageFileForCustomPath(const std::filesystem::path& nonBPKFilePath);
			
			DStorageRequestContainer& GetCurrentRequestContainer();
			const DStorageRequestContainer& GetCurrentRequestContainer() const;

		private:
			IDStorageFactory* mDStorageFactoryPtr;
			IDStorageFile* mBPKDStorageFilePtr;
			std::unordered_map<std::filesystem::path, Microsoft::WRL::ComPtr<IDStorageFile>> mDStorageFilePathMap;
			std::array<DStorageRequestContainer, std::to_underlying(DSTORAGE_PRIORITY::DSTORAGE_PRIORITY_COUNT)> mDStorageRequestContainerArr;
		};
	}
}