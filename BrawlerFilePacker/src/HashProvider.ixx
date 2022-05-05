module;
#include "Win32Def.h"

export module Brawler.HashProvider;

export namespace Brawler
{
	class HashProvider
	{
	public:
		HashProvider();
		~HashProvider();

		HashProvider(const HashProvider& rhs) = delete;
		HashProvider& operator=(const HashProvider& rhs) = delete;

		HashProvider(HashProvider&& rhs) noexcept = delete;
		HashProvider& operator=(HashProvider&& rhs) noexcept = delete;

		BCRYPT_ALG_HANDLE GetAlgorithmProvider() const;
		std::size_t GetHashObjectSize() const;

	private:
		BCRYPT_ALG_HANDLE mHAlgorithmProvider;
		std::uint32_t mHashObjectSize;
	};
}