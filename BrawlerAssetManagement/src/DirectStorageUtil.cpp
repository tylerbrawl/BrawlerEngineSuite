module;
#include <optional>
#include <atomic>
#include <cassert>
#include <DxDef.h>

module Util.DirectStorage;
import Util.General;

namespace Util
{
	namespace DirectStorage
	{
		std::optional<Microsoft::WRL::ComPtr<IDStorageFactory>> CreateDirectStorageFactory()
		{
			if constexpr (Util::General::IsDebugModeEnabled())
			{
				static std::atomic<bool> factoryCreated{ false };

				const bool wasFactoryPreviouslyCreated = factoryCreated.exchange(true, std::memory_order::relaxed);
				assert(!wasFactoryPreviouslyCreated && "ERROR: An attempt was made to call Util::DirectStorage::CreateDirectStorageFactory() more than once!");
			}

			static constexpr DSTORAGE_CONFIGURATION DEFAULT_CONFIGURATION{
				.NumSubmitThreads = 0,
				.ForceMappingLayer = false,
				.DisableBypassIO = false,

				// I didn't think Microsoft would have the balls to add telemetry to their own C++
				// APIs. I was wrong.
				.DisableTelemetry = true
			};

			// This needs to be called before the first call to DStorageGetFactory(); otherwise, the
			// default settings are used.
			HRESULT hr = DStorageSetConfiguration(&DEFAULT_CONFIGURATION);

			// For the sake of the user, don't use DirectStorage if we can't disable telemetry.
			if (FAILED(hr)) [[unlikely]]
				return std::nullopt;

			Microsoft::WRL::ComPtr<IDStorageFactory> directStorageFactory{};
			hr = DStorageGetFactory(IID_PPV_ARGS(&directStorageFactory));

			if (FAILED(hr)) [[unlikely]]
				return std::nullopt;

			if constexpr (Util::General::IsDebugModeEnabled())
			{
				static constexpr DSTORAGE_DEBUG DIRECT_STORAGE_DEBUG_LAYER_FLAGS = (DSTORAGE_DEBUG::DSTORAGE_DEBUG_SHOW_ERRORS | DSTORAGE_DEBUG::DSTORAGE_DEBUG_BREAK_ON_ERROR | DSTORAGE_DEBUG::DSTORAGE_DEBUG_RECORD_OBJECT_NAMES);
				directStorageFactory->SetDebugFlags(static_cast<std::uint32_t>(DIRECT_STORAGE_DEBUG_LAYER_FLAGS));
			}

			if constexpr (STAGING_BUFFER_SIZE_IN_BYTES != std::to_underlying(DSTORAGE_STAGING_BUFFER_SIZE::DSTORAGE_STAGING_BUFFER_SIZE_32MB))
				Util::General::CheckHRESULT(directStorageFactory->SetStagingBufferSize(STAGING_BUFFER_SIZE_IN_BYTES));

			return std::optional<Microsoft::WRL::ComPtr<IDStorageFactory>>{ std::move(directStorageFactory) };
		}
	}
}