module;
#include <array>
#include <string>
#include <span>

module Brawler.SHA512Hash;
import Util.Engine;

namespace
{
	static constexpr const char* HASH_CHAR_SELECTION_STRING = "0123456789ABCDEF";
}

namespace Brawler
{
	SHA512Hash::SHA512Hash() :
		mByteArr()
	{}

	SHA512Hash::SHA512Hash(std::array<std::uint8_t, Util::Engine::SHA_512_HASH_SIZE_IN_BYTES>&& byteArr) :
		mByteArr(std::move(byteArr))
	{}

	bool SHA512Hash::operator==(const SHA512Hash& rhs) const
	{
		for (std::size_t i = 0; i < mByteArr.size(); ++i)
		{
			if (mByteArr[i] != rhs.mByteArr[i])
				return false;
		}

		return true;
	}

	std::strong_ordering SHA512Hash::operator<=>(const SHA512Hash& rhs) const
	{
		for (std::size_t i = 0; i < mByteArr.size(); ++i)
		{
			const std::strong_ordering byteComparisonOrder{ mByteArr[i] <=> rhs.mByteArr[i] };
			
			if (byteComparisonOrder != 0)
				return byteComparisonOrder;
		}

		return std::strong_ordering::equal;
	}

	std::string SHA512Hash::ToString() const
	{
		// The MSVC uses a global critical section for all instances of std::stringstream. 
		// Trust me: This is WAY better than using std::hex() with a std::stringstream.

		std::string hashStr{};
		bool canAddZeroes = false;

		for (const auto byte : mByteArr)
		{
			// Add a character for the first four bits of the current byte.
			char currChar = HASH_CHAR_SELECTION_STRING[(byte & 0xF0) >> 4];

			if (currChar != '0' || canAddZeroes)
			{
				hashStr += currChar;
				canAddZeroes = true;
			}

			// Add a character for the last four bits of the current byte.
			currChar = HASH_CHAR_SELECTION_STRING[byte & 0xF];

			if (currChar != '0' || canAddZeroes)
			{
				hashStr += currChar;
				canAddZeroes = true;
			}
		}

		return hashStr;
	}

	std::span<const std::uint8_t, Util::Engine::SHA_512_HASH_SIZE_IN_BYTES> SHA512Hash::GetByteArray() const
	{
		return mByteArr;
	}
}