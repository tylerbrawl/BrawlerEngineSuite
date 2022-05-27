module;
#include <memory>
#include <array>
#include <DxDef.h>

export module Brawler.AssetManagement.DirectStorageAssetIORequestHandler;
import Brawler.AssetManagement.I_AssetIORequestHandler;
import Brawler.AssetManagement.EnqueuedAssetDependency;

export namespace Brawler
{
	namespace AssetManagement
	{
		class DirectStorageAssetIORequestHandler final : public I_AssetIORequestHandler
		{
		public:
			explicit DirectStorageAssetIORequestHandler(Microsoft::WRL::ComPtr<IDStorageFactory>&& directStorageFactory);

			DirectStorageAssetIORequestHandler(const DirectStorageAssetIORequestHandler& rhs) = delete;
			DirectStorageAssetIORequestHandler& operator=(const DirectStorageAssetIORequestHandler& rhs) = delete;

			DirectStorageAssetIORequestHandler(DirectStorageAssetIORequestHandler&& rhs) noexcept = default;
			DirectStorageAssetIORequestHandler& operator=(DirectStorageAssetIORequestHandler&& rhs) noexcept = default;

			void PrepareAssetIORequests(std::unique_ptr<EnqueuedAssetDependency>&& enqueuedDependency) override;

		private:
			void CreateBPKArchiveDirectStorageFile();
			void CreateDecompressionEventQueue();
			void CreateDirectStorageQueues();
			void CreateStatusArray();

		private:
			Microsoft::WRL::ComPtr<IDStorageFactory> mDStorageFactory;
			Microsoft::WRL::ComPtr<IDStorageFile> mBPKDStorageFile;
			Microsoft::WRL::ComPtr<IDStorageCustomDecompressionQueue> mDecompressionEventQueue;
			std::array<Microsoft::WRL::ComPtr<IDStorageQueue>, std::to_underlying(DSTORAGE_PRIORITY::DSTORAGE_PRIORITY_COUNT)> mDStorageQueueArr;
			Microsoft::WRL::ComPtr<IDStorageStatusArray> mDStorageStatusArray;
		};
	}
}