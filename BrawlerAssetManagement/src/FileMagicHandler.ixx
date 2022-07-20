module;
#include <cstdint>
#include <cassert>
#include <ranges>
#include <compare>

export module Brawler.FileMagicHandler;
import Brawler.NZStringView;

export namespace Brawler
{
	class FileMagicHandler
	{
	public:
		consteval explicit FileMagicHandler(const NZStringView magicStr);

		consteval FileMagicHandler(const FileMagicHandler& rhs) = default;
		consteval FileMagicHandler& operator=(const FileMagicHandler& rhs) = default;

		consteval FileMagicHandler(FileMagicHandler&& rhs) noexcept = default;
		consteval FileMagicHandler& operator=(FileMagicHandler&& rhs) noexcept = default;

		/// <summary>
		/// Retrieves the corresponding little-endian integer value for the magic string.
		/// This is the default integer value which should be used on Windows. For systems
		/// which use big-endian format, use FileMagicHandler::GetBitEndianMagicIntegerValue().
		/// 
		/// If the provided string has less than four characters, then the remaining bytes
		/// in the integer value are zeroed.
		/// </summary>
		/// <returns>
		/// The function returns the corresponding little-endian integer value for the magic
		/// string.
		/// </returns>
		consteval std::uint32_t GetMagicIntegerValue() const;

		consteval NZStringView GetMagicString() const;

		consteval std::uint32_t GetBigEndianMagicIntegerValue() const;

	private:
		NZStringView mMagicStr;
	};
}

// -------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	consteval FileMagicHandler::FileMagicHandler(const NZStringView magicStr) :
		mMagicStr(magicStr)
	{
		assert(magicStr.GetSize() <= 4 && "ERROR: An attempt was made to create a magic string which was more than 4 characters long!");
	}

	consteval std::uint32_t FileMagicHandler::GetMagicIntegerValue() const
	{
		std::uint32_t magicNumValue = 0;
		std::uint32_t bitShiftAmount = 24;

		// We reverse the bytes for little-endian serialization. Using std::views::reverse is causing
		// an MSVC crash, for some reason.
		for (auto itr = mMagicStr.rbegin(); itr != mMagicStr.rend(); ++itr)
		{
			magicNumValue |= (static_cast<std::uint32_t>(*itr) << bitShiftAmount);
			bitShiftAmount -= 8;
		}

		return magicNumValue;
	}

	consteval NZStringView FileMagicHandler::GetMagicString() const
	{
		return mMagicStr;
	}

	consteval std::uint32_t FileMagicHandler::GetBigEndianMagicIntegerValue() const
	{
		std::uint32_t magicValue = 0;
		std::uint32_t bitShiftAmount = 0;

		for (const auto c : mMagicStr)
		{
			magicValue |= (static_cast<std::uint32_t>(c) << bitShiftAmount);
			bitShiftAmount += 8;
		}

		return magicValue;
	}
}