module;
#include <array>
#include <string>
#include <ranges>
#include <compare>

export module Brawler.SHA512Hashing:SHA512Hash;
import Brawler.NZStringView;

namespace Brawler
{
	static constexpr std::size_t SHA_512_HASH_SIZE_BYTES = 64;
}

export namespace Brawler
{
	class SHA512Hash;
}

export namespace Brawler
{
	constexpr bool operator==(const Brawler::SHA512Hash& lhs, const Brawler::SHA512Hash& rhs);
}

export namespace Brawler
{
	class SHA512Hash
	{
	private:
		friend constexpr bool operator==(const SHA512Hash& lhs, const SHA512Hash& rhs);

	private:
		using SHA512ByteArray = std::array<std::byte, SHA_512_HASH_SIZE_BYTES>;

	public:
		constexpr explicit SHA512Hash(SHA512ByteArray&& byteArr);

		constexpr SHA512Hash(const SHA512Hash& rhs) = default;
		constexpr SHA512Hash& operator=(const SHA512Hash& rhs) = default;

		constexpr SHA512Hash(SHA512Hash&& rhs) noexcept = default;
		constexpr SHA512Hash& operator=(SHA512Hash&& rhs) noexcept = default;

		constexpr std::string CreateHashString() const;

	private:
		SHA512ByteArray mByteArr;
	};
}

// ------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	constexpr SHA512Hash::SHA512Hash(SHA512ByteArray&& byteArr) :
		mByteArr(std::move(byteArr))
	{}

	constexpr std::string SHA512Hash::CreateHashString() const
	{
		constexpr NZStringView HEX_BYTE_STR{ "0123456789ABCDEF" };

		std::string hashStr{ "0x" };

		for (const auto byte : mByteArr)
		{
			hashStr += HEX_BYTE_STR[(std::to_underlying(byte) >> 4)];
			hashStr += HEX_BYTE_STR[(std::to_underlying(byte) & 0xF)];
		}

		return hashStr;
	}

	constexpr bool operator==(const Brawler::SHA512Hash& lhs, const Brawler::SHA512Hash& rhs)
	{
		if (std::is_constant_evaluated())
		{
			for (std::size_t i = 0; i < Brawler::SHA_512_HASH_SIZE_BYTES; ++i)
			{
				if (lhs.mByteArr[i] != rhs.mByteArr[i])
					return false;
			}

			return true;
		}

		else
			return (std::memcmp(lhs.mByteArr.data(), rhs.mByteArr.data(), Brawler::SHA_512_HASH_SIZE_BYTES) == 0);
	}
}

// ------------------------------------------------------------------------------------------------------------------------------------------

export namespace std
{
	template <>
	struct hash<Brawler::SHA512Hash>
	{
		std::size_t operator()(const Brawler::SHA512Hash& key) const noexcept
		{
			// This is probably the least efficient way of doing this. Then again, if you were creating an
			// SHA-512 hash in the first place, then you probably care more about collisions and security than
			// performance.

			const std::hash<std::string> stringHasher{};
			return stringHasher(key.CreateHashString());
		}
	};
}