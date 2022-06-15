module;
#include <memory>
#include <cassert>
#include "DxDef.h"

export module Brawler.Win32.SafeModule;

namespace Brawler
{
	namespace Win32
	{
		struct ModuleDeleter
		{
			constexpr ModuleDeleter() = default;

			void operator()(HMODULE hModule)
			{
				// None of the DLL APIs seem to return INVALID_HANDLE_VALUE on failure, but
				// I'll include it here, just in case.
				if (hModule != nullptr && hModule != INVALID_HANDLE_VALUE) [[likely]]
				{
					const bool freeResult = FreeLibrary(hModule);
					assert(freeResult && "ERROR: FreeLibrary() failed to free a loaded DLL file!");
				}
			}
		};
	}
}

export namespace Brawler
{
	namespace Win32
	{
		using SafeModule = std::unique_ptr<std::remove_pointer_t<HMODULE>, ModuleDeleter>;
	}
}