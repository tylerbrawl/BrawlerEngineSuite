module;
#include <memory>
#include <array>
#include <atomic>
#include <DxDef.h>

export module Brawler.AssetManagement.DirectStorageAssetIORequestHandler;
import Brawler.AssetManagement.I_AssetIORequestHandler;
import Brawler.AssetManagement.EnqueuedAssetDependency;
import Brawler.AssetManagement.DirectStorageQueue;
import Brawler.ThreadSafeVector;
import Brawler.AssetManagement.PendingDirectStorageRequest;

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
			void PostPrepareAssetIORequests() override;

			IDStorageFactory& GetDirectStorageFactory();
			const IDStorageFactory& GetDirectStorageFactory() const;

		private:
			void CreateBPKArchiveDirectStorageFile();
			void CreateDecompressionEventQueue();
			void CreateDirectStorageQueues();

			void ClearCompletedRequests();

			void BeginCPUDecompression() const;
			void ExecuteCPUDecompressionTasks(const std::shared_ptr<std::atomic<std::uint32_t>>& remainingThreadsCounter) const;

		private:
			Microsoft::WRL::ComPtr<IDStorageFactory> mDStorageFactory;
			Microsoft::WRL::ComPtr<IDStorageFile> mBPKDStorageFile;
			Microsoft::WRL::ComPtr<IDStorageCustomDecompressionQueue> mDecompressionEventQueue;
			std::array<DirectStorageQueue, std::to_underlying(DSTORAGE_PRIORITY::DSTORAGE_PRIORITY_COUNT)> mDirectStorageQueueArr;
			Brawler::ThreadSafeVector<std::unique_ptr<PendingDirectStorageRequest>> mPendingRequestArr;
		};
	}
}