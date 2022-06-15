module;
#include <memory>
#include <array>
#include <atomic>

export module Brawler.AssetManagement.Win32AssetIORequestHandler;
import Brawler.AssetManagement.I_AssetIORequestHandler;
import Brawler.AssetManagement.EnqueuedAssetDependency;
import Brawler.ThreadSafeQueue;
import Brawler.ThreadSafeVector;
import Brawler.AssetManagement.Win32AssetIORequestBuilder;
import Brawler.AssetManagement.Win32AssetIORequest;
import Brawler.JobPriority;

namespace Brawler
{
	namespace AssetManagement
	{
		static constexpr std::size_t ASSET_REQUEST_QUEUE_SIZE = 1024;
	}
}

export namespace Brawler
{
	namespace AssetManagement
	{
		class Win32AssetIORequestHandler final : public I_AssetIORequestHandler
		{
		public:
			Win32AssetIORequestHandler();

			Win32AssetIORequestHandler(const Win32AssetIORequestHandler& rhs) = delete;
			Win32AssetIORequestHandler& operator=(const Win32AssetIORequestHandler& rhs) = delete;

			Win32AssetIORequestHandler(Win32AssetIORequestHandler&& rhs) noexcept = default;
			Win32AssetIORequestHandler& operator=(Win32AssetIORequestHandler&& rhs) noexcept = default;

			void PrepareAssetIORequest(EnqueuedAssetDependency&& enqueuedDependency) override;
			void SubmitAssetIORequests() override;

		private:
			void BeginAssetLoading();
			void ExecuteAssetIORequests(const std::shared_ptr<std::atomic<std::uint32_t>>& remainingThreadsCounter);

			void CreateDelayedAssetLoadingJobForCurrentThread();

		private:
			std::array<Brawler::ThreadSafeQueue<Win32AssetIORequest, ASSET_REQUEST_QUEUE_SIZE>, std::to_underlying(JobPriority::COUNT)> mRequestQueueArr;
			Brawler::ThreadSafeVector<std::unique_ptr<Win32AssetIORequestBuilder>> mActiveBuilderArr;
			std::atomic<std::uint32_t> mNumThreadsExecutingRequests;
			std::atomic<bool> mActiveRequestsExist;
		};
	}
}