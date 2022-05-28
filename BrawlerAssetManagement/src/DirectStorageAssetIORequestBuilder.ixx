module;
#include <vector>
#include <array>
#include <span>
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
			DirectStorageAssetIORequestBuilder(IDStorageFile& bpkDStorageFile);

			DirectStorageAssetIORequestBuilder(const DirectStorageAssetIORequestBuilder& rhs) = delete;
			DirectStorageAssetIORequestBuilder& operator=(const DirectStorageAssetIORequestBuilder& rhs) = delete;

			DirectStorageAssetIORequestBuilder(DirectStorageAssetIORequestBuilder&& rhs) noexcept = default;
			DirectStorageAssetIORequestBuilder& operator=(DirectStorageAssetIORequestBuilder&& rhs) noexcept = default;

			void AddAssetIORequest(const Brawler::FilePathHash pathHash, Brawler::D3D12::I_BufferSubAllocation& bufferSubAllocation) override;

			std::span<const DSTORAGE_REQUEST> GetDStorageRequestSpan(const Brawler::JobPriority priority) const;

		private:
			DSTORAGE_SOURCE CreateDStorageSourceForBPKAsset(const Brawler::FilePathHash pathHash) const;
			
			DStorageRequestContainer& GetCurrentRequestContainer();
			const DStorageRequestContainer& GetCurrentRequestContainer() const;

		private:
			IDStorageFile* mBPKDStorageFilePtr;
			std::array<DStorageRequestContainer, std::to_underlying(DSTORAGE_PRIORITY::DSTORAGE_PRIORITY_COUNT)> mDStorageRequestContainerArr;
		};
	}
}