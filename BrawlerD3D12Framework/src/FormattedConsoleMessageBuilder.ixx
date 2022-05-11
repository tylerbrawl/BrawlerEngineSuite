module;
#include <string>
#include <iostream>

export module Brawler.Win32.FormattedConsoleMessageBuilder;
import Brawler.Win32.ConsoleFormat;
import Util.General;

namespace Brawler
{
	namespace Win32
	{
		class FormattedConsoleMessageBuilder;
	}
}

export
{
	Brawler::Win32::FormattedConsoleMessageBuilder& operator<<(Brawler::Win32::FormattedConsoleMessageBuilder& lhs, const std::string_view rhs);
	Brawler::Win32::FormattedConsoleMessageBuilder& operator<<(Brawler::Win32::FormattedConsoleMessageBuilder& lhs, const std::wstring_view rhs);
	Brawler::Win32::FormattedConsoleMessageBuilder& operator<<(Brawler::Win32::FormattedConsoleMessageBuilder& lhs, const Brawler::Win32::ConsoleFormat rhs);
}

export namespace Brawler
{
	namespace Win32
	{
		/// <summary>
		/// NOTE: I initially wanted to make most of FormattedConsoleMessageBuilder constexpr, but
		/// that was making the MSVC crash with internal compiler errors. For now, this class must
		/// be used at runtime only.
		/// </summary>
		class FormattedConsoleMessageBuilder
		{
		private:
			friend FormattedConsoleMessageBuilder& (::operator<<)(FormattedConsoleMessageBuilder& lhs, const std::string_view rhs);
			friend FormattedConsoleMessageBuilder& (::operator<<)(FormattedConsoleMessageBuilder& lhs, const std::wstring_view rhs);
			friend FormattedConsoleMessageBuilder& (::operator<<)(FormattedConsoleMessageBuilder& lhs, const Brawler::Win32::ConsoleFormat rhs);

		public:
			explicit FormattedConsoleMessageBuilder(const ConsoleFormat defaultFormat = ConsoleFormat::NORMAL);

			// Although there is nothing technically wrong with copying std::wstring instances, 
			// doing so is expensive, so we disable copying.
			FormattedConsoleMessageBuilder(const FormattedConsoleMessageBuilder& rhs) = delete;
			FormattedConsoleMessageBuilder& operator=(const FormattedConsoleMessageBuilder& rhs) = delete;

			FormattedConsoleMessageBuilder(FormattedConsoleMessageBuilder&& rhs) noexcept = default;
			FormattedConsoleMessageBuilder& operator=(FormattedConsoleMessageBuilder&& rhs) noexcept = default;

			const std::wstring_view GetFormattedConsoleMessage() const;

			/// <summary>
			/// Writes the generated message out to STDOUT, along with an additional newline character.
			/// This function is thread safe; calling FormattedConsoleMessageBuilder::WriteFormattedConsoleMessage()
			/// on multiple FormattedConsoleMessageBuilder instances across multiple threads will not
			/// break the formatting of messages.
			/// </summary>
			void WriteFormattedConsoleMessage() const;

		private:
			// We don't use std::stringstream because it is implemented with a global critical
			// section in the MSVC STL.
			std::wstring mFormattedStr;
		};
	}
}

export
{
	Brawler::Win32::FormattedConsoleMessageBuilder& operator<<(Brawler::Win32::FormattedConsoleMessageBuilder& lhs, const std::string_view rhs);
	Brawler::Win32::FormattedConsoleMessageBuilder& operator<<(Brawler::Win32::FormattedConsoleMessageBuilder& lhs, const std::wstring_view rhs);
	Brawler::Win32::FormattedConsoleMessageBuilder& operator<<(Brawler::Win32::FormattedConsoleMessageBuilder& lhs, const Brawler::Win32::ConsoleFormat rhs);
}

// ----------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace Win32
	{
		FormattedConsoleMessageBuilder::FormattedConsoleMessageBuilder(const Brawler::Win32::ConsoleFormat defaultFormat) :
			mFormattedStr(GetConsoleFormatString(defaultFormat))
		{}

		const std::wstring_view FormattedConsoleMessageBuilder::GetFormattedConsoleMessage() const
		{
			return std::wstring_view{ mFormattedStr };
		}

		void FormattedConsoleMessageBuilder::WriteFormattedConsoleMessage() const
		{
			// We don't want to introduce a race condition, so we need to send only one string
			// out to std::wcout. This is why we create a new std::wstring before writing to the stream.
			// Had we done multiple writes, it would be possible for other threads to write in the
			// middle of another thread's write, resulting in the formatting of messages becoming messed
			// up.
			
			const std::wstring formattedMsg{ mFormattedStr + Brawler::Win32::GetConsoleFormatString(Brawler::Win32::ConsoleFormat::NORMAL) + L"\n" };
			std::wcout << formattedMsg;
		}
	}
}

Brawler::Win32::FormattedConsoleMessageBuilder& operator<<(Brawler::Win32::FormattedConsoleMessageBuilder& lhs, const std::string_view rhs)
{
	lhs.mFormattedStr += Util::General::StringToWString(rhs);
	return lhs;
}

Brawler::Win32::FormattedConsoleMessageBuilder& operator<<(Brawler::Win32::FormattedConsoleMessageBuilder& lhs, const std::wstring_view rhs)
{
	lhs.mFormattedStr += std::wstring{ rhs };
	return lhs;
}

Brawler::Win32::FormattedConsoleMessageBuilder& operator<<(Brawler::Win32::FormattedConsoleMessageBuilder& lhs, const Brawler::Win32::ConsoleFormat rhs)
{
	lhs.mFormattedStr += GetConsoleFormatString(rhs);
	return lhs;
}