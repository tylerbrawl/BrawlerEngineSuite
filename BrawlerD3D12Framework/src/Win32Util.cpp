module;
#include <string>
#include <iostream>
#include <stdexcept>
#include <cassert>
#include <format>
#include <optional>
#include <filesystem>
#include <io.h>
#include <fcntl.h>
#include "DxDef.h"

#include <objbase.h>
#include <comdef.h>

module Util.Win32;
import Util.General;
import Brawler.Win32.SafeHandle;

namespace
{
	void EnableConsoleFormatting()
	{
		// Here, we try to modify the application's console to support virtual terminal characters
		// and foreign characters. However, we don't throw an exception if we fail here, because
		// this is just for aesthetics; execution can continue normally if we cannot complete
		// these modifications successfully.
		
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

	void RaiseFileStreamCapacity()
	{
		// By default, the maximum number of files which can be opened at once with std::fopen() (and
		// thus with std::fstream instances) is 512. However, this limit can be raised to 8,192 by calling
		// _setmaxstdio(). (The source for this information is 
		// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/setmaxstdio?view=msvc-170.)
		static constexpr std::uint32_t MAX_FILE_STREAMS_ALLOWED = 8192;

		const std::int32_t setMaxReturnValue = _setmaxstdio(static_cast<std::int32_t>(MAX_FILE_STREAMS_ALLOWED));

		if (setMaxReturnValue == -1) [[unlikely]]
			throw std::runtime_error{ "ERROR: The maximum number of open file streams could not be raised!" };
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
			RaiseFileStreamCapacity();
		}

		void WriteDebugMessage(const std::string_view msg)
		{
			if constexpr (Util::General::IsDebugModeEnabled())
				WriteDebugMessage(Util::General::StringToWString(msg));
		}

		void WriteDebugMessage(const std::wstring_view msg)
		{
			if constexpr (Util::General::IsDebugModeEnabled())
			{
				const std::wstring formattedMsg{ std::format(L"{}\n", msg) };
				OutputDebugString(formattedMsg.c_str());
			}
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

			const std::wstring formattedMsg{ Brawler::Win32::GetConsoleFormatString(format) + std::wstring{ msg } + Brawler::Win32::GetConsoleFormatString(ConsoleFormat::NORMAL) + L"\n" };
			std::wcout << formattedMsg;
		}

		std::wstring GetLastErrorString()
		{
			LPWSTR messageStrBuffer = nullptr;
			
			const std::uint32_t formatMessageResult = FormatMessage(
				FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
				nullptr,
				GetLastError(),
				0,
				reinterpret_cast<LPWSTR>(&messageStrBuffer),
				0,
				nullptr
			);

			if (formatMessageResult == 0) [[unlikely]]
				throw std::runtime_error{ "ERROR: FormatMessage() failed to allocate a buffer to get the string equivalent of a GetLastError() error code!" };

			const std::wstring errMsgString{ messageStrBuffer };

			{
				const HLOCAL hFreeResult = LocalFree(messageStrBuffer);

				if (hFreeResult != nullptr) [[unlikely]]
					throw std::runtime_error{ "ERROR: LocalFree() failed to deallocate the buffer allocated by FormatMessage()!" };
			}
			
			return errMsgString;
		}

		std::optional<std::filesystem::path> GetKnownFolderPath(const KNOWNFOLDERID& folderID, const bool createIfNotFound)
		{
			PWSTR pFolderLocation = nullptr;
			KNOWN_FOLDER_FLAG dwFlags = KNOWN_FOLDER_FLAG::KF_FLAG_DEFAULT;

			if (createIfNotFound) [[unlikely]]
				dwFlags |= KNOWN_FOLDER_FLAG::KF_FLAG_CREATE;

			const HRESULT hr = SHGetKnownFolderPath(
				folderID,
				dwFlags,
				nullptr,
				&pFolderLocation
			);

			if (FAILED(hr)) [[unlikely]]
			{
				CoTaskMemFree(pFolderLocation);
				return std::optional<std::filesystem::path>{};
			}

			std::filesystem::path knownFolderPath{ pFolderLocation };
			CoTaskMemFree(pFolderLocation);

			return std::optional<std::filesystem::path>{ std::move(knownFolderPath) };
		}
	}
}