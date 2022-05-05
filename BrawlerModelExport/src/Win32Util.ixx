module;
#include <string>
#include "DxDef.h"

export module Util.Win32;

export namespace Util
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

		void EnableConsoleFormatting();
		void WriteFormattedConsoleMessage(const std::string_view msg, const ConsoleFormat format = Util::Win32::ConsoleFormat::NORMAL);
		void WriteFormattedConsoleMessage(const std::wstring_view msg, const ConsoleFormat format = Util::Win32::ConsoleFormat::NORMAL);

		void InitializeCOM();
	}
}