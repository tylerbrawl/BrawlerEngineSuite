module;
#include <vector>
#include <span>
#include <Windows.h>
#include <bcrypt.h>

export module Brawler.SHA512Hashing:SHA512Hasher;
import :SHA512Hash;

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

		SHA512Hash HashData(const std::span<const std::byte> srcDataSpan) const;

	private:
		void DeleteHashObject();

	private:
		BCRYPT_HASH_HANDLE mHHashObject;
		std::vector<std::byte> mHashObjectBackingMemory;
	};
}