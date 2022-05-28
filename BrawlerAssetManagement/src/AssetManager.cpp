module;
#include <atomic>
#include <memory>
#include <optional>
#include <cassert>
#include <thread>
#include <DxDef.h>

module Brawler.AssetManagement.AssetManager;
import Brawler.AssetManagement.DirectStorageAssetIORequestHandler;
import Brawler.AssetManagement.Win32AssetIORequestHandler;
import Util.DirectStorage;

namespace
{
	// Use this to test Win32 file I/O on a system where the Brawler Engine would otherwise prefer to use
	// DirectStorage.
	static constexpr bool FORCE_WIN32_ASSET_REQUEST_HANDLER_USE = false;
}

namespace Brawler
{
	namespace AssetManagement
	{
		AssetManager::AssetManager() :
			mRequestHandlerPtr(nullptr),
			mCurrLoadingMode(AssetLoadingMode::MINIMAL_OVERHEAD),
			mDependencyQueue()
		{
			// Looking at the function signature for DStorageGetFactory(), we find that the function returns
			// an HRESULT value. This implies that the function can, in fact, fail. However, the documentation
			// does not state when the function actually fails to create an IDStorageFactory instance.
			//
			// We'll just err on the side of caution, then, and assume that there are some systems out there
			// which can run Direct3D 12, but not DirectStorage. For these systems, we will fall back to
			// using the standard Win32 API for file I/O. This is why we use dynamic polymorphism for
			// mRequestHandlerPtr.
			if constexpr (!FORCE_WIN32_ASSET_REQUEST_HANDLER_USE)
			{
				std::optional<Microsoft::WRL::ComPtr<IDStorageFactory>> directStorageFactory{ Util::DirectStorage::CreateDirectStorageFactory() };
				
				if (directStorageFactory.has_value()) [[likely]]
					mRequestHandlerPtr = std::make_unique<DirectStorageAssetIORequestHandler>(std::move(*directStorageFactory));
				//else [[unlikely]]
					//mRequestHandlerPtr = std::make_unique<Win32AssetIORequestHandler>();
			}
			else
				mRequestHandlerPtr = std::make_unique<Win32AssetIORequestHandler>();
		}

		AssetManager& AssetManager::GetInstance()
		{
			static AssetManager instance{};
			return instance;
		}

		AssetRequestEventHandle AssetManager::EnqueueAssetDependency(AssetDependency&& dependency)
		{
			// Create the event handle here. We'll need to copy it into the queue's unique_ptr.
			AssetRequestEventHandle hRequestEvent{};
			std::unique_ptr<EnqueuedAssetDependency> enqueuedDependencyPtr{ std::make_unique<EnqueuedAssetDependency>(std::move(dependency), hRequestEvent) };

			// If we cannot enqueue the request, then we need to begin processing it now.
			if (!mDependencyQueue.PushBack(std::move(enqueuedDependencyPtr))) [[unlikely]]
				HandleEnqueuedAssetDependency(std::move(enqueuedDependencyPtr));

			return hRequestEvent;
		}

		void AssetManager::SetAssetLoadingMode(const AssetLoadingMode loadingMode)
		{
			assert(loadingMode != AssetLoadingMode::COUNT_OR_ERROR && "ERROR: An invalid AssetLoadingMode value was specified in a call to AssetManager::SetAssetLoadingMode()!");
			mCurrLoadingMode.store(loadingMode, std::memory_order::relaxed);
		}

		AssetLoadingMode AssetManager::GetAssetLoadingMode() const
		{
			return mCurrLoadingMode.load(std::memory_order::relaxed);
		}

		void AssetManager::HandleEnqueuedAssetDependency(std::unique_ptr<EnqueuedAssetDependency>&& enqueuedDependency) const
		{

		}
	}
}