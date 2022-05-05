module;
#include <string>
#include <iostream>
#include <io.h>
#include <fcntl.h>
#include "Win32Def.h"

module Util.Win32;
import Util.General;

namespace
{
	constexpr std::wstring GetConsoleFormatString(const Util::Win32::ConsoleFormat format)
	{
		// Visit the MSDN at https://docs.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences#text-formatting
		// for documentation regarding the various virtual terminal sequences used for
		// text formatting.

		switch (format)
		{
		case Util::Win32::ConsoleFormat::SUCCESS:
			return std::wstring{ L"\x1b[22;32m" };  // Normal Green Foreground Text

		case Util::Win32::ConsoleFormat::WARNING:
			return std::wstring{ L"\x1b[22;33m" };  // Normal Yellow Foreground Text

		case Util::Win32::ConsoleFormat::CRITICAL_FAILURE:
			return std::wstring{ L"\x1b[1;31m" };   // Bold Red Foreground Text

		case Util::Win32::ConsoleFormat::NORMAL:
			[[fallthrough]];

		default:
			return std::wstring{ L"\x1b[0m" };	  // Default Text Format
		}
	}
}

namespace Util
{
	namespace Win32
	{
		__forceinline constexpr bool NT_SUCCESS(const NTSTATUS status)
		{
			return (static_cast<std::uint32_t>(status) <= 0x7FFFFFFF);
		}

		void EnableConsoleFormatting()
		{
			HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
			if (!hStdOut) [[unlikely]]
				return;

			DWORD dwConsoleMode = 0;
			if (!GetConsoleMode(hStdOut, &dwConsoleMode)) [[unlikely]]
				return;

			dwConsoleMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
			SetConsoleMode(hStdOut, dwConsoleMode);

			// Set the file translation mode to UTF-16. That way, we can "properly" handle foreign characters.
			//
			// ...Well, that's what I would like to happen, anyways. In reality, not all characters
			// can be printed out by the Windows Command Prompt properly. For instance, Japanese
			// characters refuse to print as anything other than rectangles if the locale is not
			// properly set.
			_setmode(_fileno(stdout), _O_U16TEXT);
		}

		void WriteFormattedConsoleMessage(const std::string_view msg, const ConsoleFormat format)
		{
			WriteFormattedConsoleMessage(Util::General::StringToWString(msg), format);
		}

		void WriteFormattedConsoleMessage(const std::wstring_view msg, const ConsoleFormat format)
		{
			// Rather than using SetConsoleTextAttribute(), we make use of virtual terminal characters -
			// but not because the MSDN recommends it, nor because we want to. We do it like this for
			// the sake of multi-threading: All format changes using virtual terminal characters can
			// be atomic without the need for us to use critical sections. (Windows might itself use
			// these, but that isn't for us to worry about.)

			const std::wstring formattedMsg{ GetConsoleFormatString(format) + std::wstring{ msg } + GetConsoleFormatString(ConsoleFormat::NORMAL) + L"\n" };
			std::wcout << formattedMsg;
		}
	}
}