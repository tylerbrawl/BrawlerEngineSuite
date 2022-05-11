module;
#include <string>
#include <iostream>
#include <stdexcept>
#include <io.h>
#include <fcntl.h>
#include "DxDef.h"

#include <objbase.h>
#include <comdef.h>

module Util.Win32;
import Util.General;

namespace
{
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

	void InitializeCOM()
	{
		Util::General::CheckHRESULT(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
	}
}

namespace Util
{
	namespace Win32
	{
		void InitializeWin32Components()
		{
			EnableConsoleFormatting();
			InitializeCOM();
		}

		void WriteFormattedConsoleMessage(const std::string_view msg, const Brawler::Win32::ConsoleFormat format)
		{
			WriteFormattedConsoleMessage(Util::General::StringToWString(msg), format);
		}

		void WriteFormattedConsoleMessage(const std::wstring_view msg, const Brawler::Win32::ConsoleFormat format)
		{
			// Rather than using SetConsoleTextAttribute(), we make use of virtual terminal characters -
			// but not because the MSDN recommends it, nor because we want to. We do it like this for
			// the sake of multi-threading: All format changes using virtual terminal characters can
			// be atomic without the need for us to use critical sections. (Windows might itself use
			// these, but that isn't for us to worry about.)

			const std::wstring formattedMsg{ Brawler::Win32::GetConsoleFormatString(format) + std::wstring{ msg } + Brawler::Win32::GetConsoleFormatString(Brawler::Win32::ConsoleFormat::NORMAL) + L"\n" };
			std::wcout << formattedMsg;
		}
	}
}