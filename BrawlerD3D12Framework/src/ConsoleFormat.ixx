module;
#include <string>

export module Brawler.Win32.ConsoleFormat;

export namespace Brawler
{
	namespace Win32
	{
		enum class ConsoleFormat
		{
			NORMAL,
			SUCCESS,
			WARNING,
			CRITICAL_FAILURE  // We can't use ERROR because that's a macro in the Windows headers. *sigh*...
		};

		constexpr std::wstring GetConsoleFormatString(const Brawler::Win32::ConsoleFormat format)
		{
			// Visit the MSDN at https://docs.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences#text-formatting
			// for documentation regarding the various virtual terminal sequences used for
			// text formatting.

			switch (format)
			{
			case Brawler::Win32::ConsoleFormat::SUCCESS:
				return std::wstring{ L"\x1b[22;32m" };  // Normal Green Foreground Text

			case Brawler::Win32::ConsoleFormat::WARNING:
				return std::wstring{ L"\x1b[22;33m" };  // Normal Yellow Foreground Text

			case Brawler::Win32::ConsoleFormat::CRITICAL_FAILURE:
				return std::wstring{ L"\x1b[1;31m" };   // Bold Red Foreground Text

			case Brawler::Win32::ConsoleFormat::NORMAL: [[fallthrough]];
			default:
				return std::wstring{ L"\x1b[0m" };	  // Default Text Format
			}
		}
	}
}