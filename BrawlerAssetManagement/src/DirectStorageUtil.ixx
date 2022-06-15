module;
#include <optional>
#include <algorithm>
#include <DxDef.h>

export module Util.DirectStorage;

namespace Util
{
	namespace DirectStorage
	{
		// This value might require some experimentation.
		static constexpr std::uint16_t DESIRED_DIRECT_STORAGE_QUEUE_CAPACITY = (DSTORAGE_MAX_QUEUE_CAPACITY / 2);
	}
}

export namespace Util
{
	namespace DirectStorage
	{
		constexpr std::uint16_t DIRECT_STORAGE_QUEUE_CAPACITY = std::clamp<std::uint16_t>(DESIRED_DIRECT_STORAGE_QUEUE_CAPACITY, DSTORAGE_MIN_QUEUE_CAPACITY, DSTORAGE_MAX_QUEUE_CAPACITY);
		
		/// <summary>
		/// This is the size of the staging buffers created for DirectStorage requests. If it is not
		/// equal to the default staging buffer size, Util::DirectStorage::CreateDirectStorageFactory()
		/// sets the staging buffer size accordingly before returning any IDStorageFactory instance.
		/// </summary>
		constexpr std::uint32_t STAGING_BUFFER_SIZE_IN_BYTES = std::to_underlying(DSTORAGE_STAGING_BUFFER_SIZE::DSTORAGE_STAGING_BUFFER_SIZE_32MB);
		
		std::optional<Microsoft::WRL::ComPtr<IDStorageFactory>> CreateDirectStorageFactory();
	}
}