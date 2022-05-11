module;
#include <string>
#include "DxDef.h"

export module Util.Win32;
import Brawler.Win32.ConsoleFormat;

export namespace Util
{
	namespace Win32
	{
		void InitializeWin32Components();

		void WriteFormattedConsoleMessage(const std::string_view msg, const Brawler::Win32::ConsoleFormat format = Brawler::Win32::ConsoleFormat::NORMAL);
		void WriteFormattedConsoleMessage(const std::wstring_view msg, const Brawler::Win32::ConsoleFormat format = Brawler::Win32::ConsoleFormat::NORMAL);

		__forceinline constexpr bool IsHandleValid(const HANDLE hObject);
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
	}
}