module;
#include <vector>
#include <span>
#include "Win32Def.h"

export module Brawler.SHA512Hasher;
import Brawler.SHA512Hash;

export namespace Brawler
{
	class HashProvider;
}

export namespace Brawler
{
	class SHA512Hasher
	{
	public:
		SHA512Hasher();
		~SHA512Hasher();

		SHA512Hasher(const SHA512Hasher& rhs) = delete;
		SHA512Hasher& operator=(const SHA512Hasher& rhs) = delete;

		SHA512Hasher(SHA512Hasher&& rhs) noexcept;
		SHA512Hasher& operator=(SHA512Hasher&& rhs) noexcept;

		void Initialize();

		SHA512Hash CreateSHA512Hash(const std::span<std::uint8_t> byteArr) const;

	private:
		void DeleteHashObject();

	private:
		BCRYPT_HASH_HANDLE mHHashObject;
		std::vector<std::uint8_t> mHashObjectBuffer;
	};
}