module;
#include <optional>
#include <DxDef.h>

export module Util.DirectStorage;

export namespace Util
{
	namespace DirectStorage
	{
		std::optional<Microsoft::WRL::ComPtr<IDStorageFactory>> CreateDirectStorageFactory();
	}
}