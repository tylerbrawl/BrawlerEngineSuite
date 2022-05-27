module;
#include <memory>
#include <atomic>

export module Brawler.AssetManagement.AssetManager;
import Brawler.AssetManagement.I_AssetIORequestHandler;
import Brawler.AssetManagement.AssetLoadingMode;
import Brawler.ThreadSafeQueue;
import Brawler.AssetManagement.EnqueuedAssetDependency;
import Brawler.AssetManagement.AssetRequestEventHandle;

export namespace Brawler
{
	namespace AssetManagement
	{
		class AssetDependency;
	}
}

namespace Brawler
{
	namespace AssetManagement
	{
		// We can make this large because the AssetManager instance is static, and thus
		// its underlying array is located in static memory.
		static constexpr std::size_t ENQUEUED_DEPENDENCY_QUEUE_SIZE = 256;
	}
}

export namespace Brawler
{
	namespace AssetManagement
	{
		class AssetManager final
		{
		private:
			AssetManager();

		public:
			~AssetManager() = default;

			AssetManager(const AssetManager& rhs) = delete;
			AssetManager& operator=(const AssetManager& rhs) = delete;

			AssetManager(AssetManager&& rhs) noexcept = default;
			AssetManager& operator=(AssetManager&& rhs) noexcept = default;

			static AssetManager& GetInstance();

			AssetRequestEventHandle EnqueueAssetDependency(AssetDependency&& dependency);

			void SetAssetLoadingMode(const AssetLoadingMode loadingMode);
			AssetLoadingMode GetAssetLoadingMode() const;

		private:
			void HandleEnqueuedAssetDependency(std::unique_ptr<EnqueuedAssetDependency>&& enqueuedDependency) const;

		private:
			/// <summary>
			/// Rather than using dynamic polymorphism, we could use something like the PolymorphicAdapter.
			/// However, since the actual object is only created once and its type never changes, the
			/// constant pointless branching seems like it would do more harm than a single heap allocation
			/// and a pointer indirection.
			/// 
			/// Don't get me wrong, though: I love the idea of static polymorphism when it makes sense. It's
			/// just that we don't know at compile time what the best I_AssetIORequestHandler is for a user's
			/// system.
			/// </summary>
			std::unique_ptr<I_AssetIORequestHandler> mRequestHandlerPtr;

			std::atomic<AssetLoadingMode> mCurrLoadingMode;
			Brawler::ThreadSafeQueue<std::unique_ptr<EnqueuedAssetDependency>, ENQUEUED_DEPENDENCY_QUEUE_SIZE> mDependencyQueue;
		};
	}
}