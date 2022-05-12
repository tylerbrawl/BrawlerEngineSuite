module;
#include <stdexcept>
#include <string>
#include <Windows.h>

module Util.General;

namespace Util
{
	namespace General
	{
		// NOTE: std::string_view and std::wstring_view are *NEVER* guaranteed to include a null-terminating
		// character! Thus, we need to explicitly use their size() functions to determine how many characters
		// to translate. Failing to do so can lead to more characters than necessary being translated... or
		// worse.

		std::wstring StringToWString(const std::string_view str)
		{
			if (str.empty()) [[unlikely]]
				return std::wstring{};

			std::int32_t wideCharsNeeded = MultiByteToWideChar(
				CP_UTF8,
				0,
				str.data(),
				static_cast<std::int32_t>(str.size()),
				nullptr,
				0
			);

			if (!wideCharsNeeded) [[unlikely]]
				throw std::runtime_error{ "MultiByteToWideChar() failed to get the number of required characters!" };

			// Add one to account for the null-terminating character.
			++wideCharsNeeded;

			wchar_t* wStrBuffer = new wchar_t[wideCharsNeeded];
			ZeroMemory(wStrBuffer, wideCharsNeeded * sizeof(wchar_t));

			std::int32_t result = MultiByteToWideChar(
				CP_UTF8,
				0,
				str.data(),
				static_cast<std::int32_t>(str.size()),
				wStrBuffer,
				wideCharsNeeded
			);

			if (result != (wideCharsNeeded - 1)) [[unlikely]]
				throw std::runtime_error{ "MultiByteToWideChar() failed to convert a string!" };

			std::wstring wideStr{ wStrBuffer };
			delete[] wStrBuffer;

			return wideStr;
		}

		std::string WStringToString(const std::wstring_view wStr)
		{
			if (wStr.empty()) [[unlikely]]
				return std::string{};

			std::int32_t byteCharsNeeded = WideCharToMultiByte(
				CP_UTF8,
				0,
				wStr.data(),
				static_cast<std::int32_t>(wStr.size()),
				nullptr,
				0,
				nullptr,
				nullptr
			);

			if (!byteCharsNeeded) [[unlikely]]
				throw std::runtime_error{ "WideCharToMultiByte() failed to get the number of required characters!" };

			// Add one to account for the null-terminating character.
			++byteCharsNeeded;

			char* strBuffer = new char[byteCharsNeeded];
			ZeroMemory(strBuffer, byteCharsNeeded);

			std::int32_t result = WideCharToMultiByte(
				CP_UTF8,
				0,
				wStr.data(),
				static_cast<std::int32_t>(wStr.size()),
				strBuffer,
				byteCharsNeeded,
				nullptr,
				nullptr
			);

			if (result != (byteCharsNeeded - 1)) [[unlikely]]
				throw std::runtime_error{ "WideCharToMultiByte() failed to convert a string!" };

			std::string str{ strBuffer };
			delete[] strBuffer;

			return str;
		}
	}
}