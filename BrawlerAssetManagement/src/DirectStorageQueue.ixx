module;
#include <mutex>
#include <DxDef.h>

export module Brawler.AssetManagement.DirectStorageQueue;
import Brawler.ThreadSafeQueue;
import Brawler.JobPriority;
import Brawler.AssetManagement.PendingDirectStorageRequest;

export namespace Brawler
{
	namespace AssetManagement
	{
		class DirectStorageAssetIORequestHandler;
	}
}

export namespace Brawler
{
	namespace AssetManagement
	{
		class DirectStorageQueue
		{
		public:
			DirectStorageQueue() = default;
			DirectStorageQueue(DirectStorageAssetIORequestHandler& owningRequestHandler, const Brawler::JobPriority queuePriority);

			DirectStorageQueue(const DirectStorageQueue& rhs) = delete;
			DirectStorageQueue& operator=(const DirectStorageQueue& rhs) = delete;

			DirectStorageQueue(DirectStorageQueue&& rhs) noexcept;
			DirectStorageQueue& operator=(DirectStorageQueue&& rhs) noexcept;

			void EnqueueRequest(PendingDirectStorageRequest& directStorageRequest);
			void Submit();

		private:
			void CreateDStorageQueue();
			void CreateDStorageStatusArray();

		private:
			DirectStorageAssetIORequestHandler* mOwningRequestHandlerPtr;
			Microsoft::WRL::ComPtr<IDStorageStatusArray> mDStorageStatusArray;
			Microsoft::WRL::ComPtr<IDStorageQueue> mDStorageQueue;
			Brawler::JobPriority mQueuePriority;
			std::uint32_t mCurrStatusArrayEntryIndex;
			bool mHasRequestsToSubmit;
			mutable std::mutex mSubmissionCritSection;
		};
	}
}