module;
#include <cstdint>
#include <cassert>
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

		consteval std::uint32_t GetMagicIntegerValue() const;
		consteval NZStringView GetMagicString() const;

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

		for (const auto c : mMagicStr)
		{
			magicNumValue |= (static_cast<std::uint32_t>(c) << bitShiftAmount);
			bitShiftAmount -= 8;
		}

		return magicNumValue;
	}

	consteval NZStringView FileMagicHandler::GetMagicString() const
	{
		return mMagicStr;
	}
}