module;
#include <memory>
#include <cassert>
#include "DxDef.h"

export module Brawler.Win32.SafeHandle;

namespace Brawler
{
	namespace Win32
	{
		struct HandleDeleter
		{
			void operator()(HANDLE hObject) const
			{
				if (hObject != nullptr && hObject != INVALID_HANDLE_VALUE) [[likely]]
				{
					const BOOL closeResult = CloseHandle(hObject);
					assert(closeResult && "ERROR: CloseHandle() failed!");
				}
			}
		};
	}
}

export namespace Brawler
{
	namespace Win32
	{
		using SafeHandle = std::unique_ptr<std::remove_pointer_t<HANDLE>, HandleDeleter>;
	}
}

export
{
	bool operator==(const Brawler::Win32::SafeHandle& lhs, const HANDLE rhs)
	{
		return (lhs.get() == rhs);
	}

	bool operator==(const HANDLE lhs, const Brawler::Win32::SafeHandle& rhs)
	{
		return (rhs == lhs);
	}
}