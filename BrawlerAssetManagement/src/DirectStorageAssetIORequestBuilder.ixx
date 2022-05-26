module;

export module Brawler.AssetManagement.DirectStorageAssetIORequestBuilder;
import Brawler.AssetManager.I_AssetIORequestBuilder;
import Brawler.FilePathHash;
import Brawler.D3D12.I_BufferSubAllocation;

export namespace Brawler
{
	namespace AssetManagement
	{
		class DirectStorageAssetIORequestBuilder final : public I_AssetIORequestBuilder
		{
		public:
			DirectStorageAssetIORequestBuilder() = default;

			DirectStorageAssetIORequestBuilder(const DirectStorageAssetIORequestBuilder& rhs) = delete;
			DirectStorageAssetIORequestBuilder& operator=(const DirectStorageAssetIORequestBuilder& rhs) = delete;

			DirectStorageAssetIORequestBuilder(DirectStorageAssetIORequestBuilder&& rhs) noexcept = default;
			DirectStorageAssetIORequestBuilder& operator=(DirectStorageAssetIORequestBuilder&& rhs) noexcept = default;

			void AddAssetIORequest(const Brawler::FilePathHash pathHash, Brawler::D3D12::I_BufferSubAllocation& bufferSubAllocation) override;
		};
	}
}