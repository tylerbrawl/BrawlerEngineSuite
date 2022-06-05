module;
#include <string>
#include "DxDef.h"

export module Util.Win32;
import Brawler.Win32.ConsoleFormat;
import Brawler.Win32.SafeHandle;
import Brawler.NZStringView;

export namespace Util
{
	namespace Win32
	{
		void InitializeWin32Components();

		void WriteDebugMessage(const std::string_view msg);
		void WriteDebugMessage(const std::wstring_view msg);
		
		void WriteFormattedConsoleMessage(const std::string_view msg, const Brawler::Win32::ConsoleFormat format = Brawler::Win32::ConsoleFormat::NORMAL);
		void WriteFormattedConsoleMessage(const std::wstring_view msg, const Brawler::Win32::ConsoleFormat format = Brawler::Win32::ConsoleFormat::NORMAL);

		std::wstring GetLastErrorString();

		__forceinline constexpr bool IsHandleValid(const HANDLE hObject);
		__forceinline constexpr bool IsHandleValid(const Brawler::Win32::SafeHandle& hObject);
	}
}

// -----------------------------------------------------------------------------------------------------------------

namespace Util
{
	namespace Win32
	{
		__forceinline constexpr bool IsHandleValid(const HANDLE hObject)
		{
			return (hObject != nullptr && hObject != INVALID_HANDLE_VALUE);
		}

		__forceinline constexpr bool IsHandleValid(const Brawler::Win32::SafeHandle& hObject)
		{
			return IsHandleValid(hObject.get());
		}
	}
}