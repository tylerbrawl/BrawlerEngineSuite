module;
#include <optional>
#include <DxDef.h>

export module Util.DirectStorage;

export namespace Util
{
	namespace DirectStorage
	{
		/// <summary>
		/// This is the size of the staging buffers created for DirectStorage requests. If it is not
		/// equal to the default staging buffer size, Util::DirectStorage::CreateDirectStorageFactory()
		/// sets the staging buffer size accordingly before returning any IDStorageFactory instance.
		/// </summary>
		constexpr std::uint32_t STAGING_BUFFER_SIZE_IN_BYTES = std::to_underlying(DSTORAGE_STAGING_BUFFER_SIZE::DSTORAGE_STAGING_BUFFER_SIZE_32MB);
		
		std::optional<Microsoft::WRL::ComPtr<IDStorageFactory>> CreateDirectStorageFactory();
	}
}