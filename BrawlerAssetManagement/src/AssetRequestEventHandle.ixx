module;
#include <memory>
#include <atomic>

export module Brawler.AssetManagement.AssetRequestEventHandle;

export namespace Brawler
{
	namespace AssetManagement
	{
		class AssetManager;
	}
}

export namespace Brawler
{
	namespace AssetManagement
	{
		class AssetRequestEventHandle
		{
		private:
			friend class AssetManager;

		private:
			AssetRequestEventHandle();

		public:
			~AssetRequestEventHandle() = default;

			AssetRequestEventHandle(const AssetRequestEventHandle& rhs) = default;
			AssetRequestEventHandle& operator=(const AssetRequestEventHandle& rhs) = default;

			AssetRequestEventHandle(AssetRequestEventHandle&& rhs) noexcept = default;
			AssetRequestEventHandle& operator=(AssetRequestEventHandle&& rhs) noexcept = default;

			bool IsAssetRequestComplete() const;

		private:
			void MarkAssetRequestAsCompleted();

		private:
			std::shared_ptr<std::atomic<bool>> mAssetRequestFinished;
		};
	}
}