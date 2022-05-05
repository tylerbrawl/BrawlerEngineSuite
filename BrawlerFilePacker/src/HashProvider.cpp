module;
#include <cassert>
#include "Win32Def.h"

module Brawler.HashProvider;
import Util.Win32;
import Util.Engine;

namespace Brawler
{
	HashProvider::HashProvider() :
		mHAlgorithmProvider(nullptr),
		mHashObjectSize(0)
	{
		NTSTATUS status = BCryptOpenAlgorithmProvider(
			&mHAlgorithmProvider,
			BCRYPT_SHA512_ALGORITHM,
			nullptr,
			BCRYPT_HASH_REUSABLE_FLAG
		);
		assert(Util::Win32::NT_SUCCESS(status));

		ULONG numBytesWritten = 0;
		status = BCryptGetProperty(
			mHAlgorithmProvider,
			BCRYPT_OBJECT_LENGTH,
			reinterpret_cast<PUCHAR>(&mHashObjectSize),
			sizeof(mHashObjectSize),
			&numBytesWritten,
			0
		);
		assert(Util::Win32::NT_SUCCESS(status));

#ifdef _DEBUG
		// Let's just make sure that the Next Generation Cryptography API isn't doing anything weird
		// with the size of an SHA-512 hash...
		std::uint32_t hashSizeInBytes = 0;
		status = BCryptGetProperty(
			mHAlgorithmProvider,
			BCRYPT_HASH_LENGTH,
			reinterpret_cast<PUCHAR>(&hashSizeInBytes),
			sizeof(hashSizeInBytes),
			&numBytesWritten,
			0
		);
		assert(Util::Win32::NT_SUCCESS(status));

		assert(hashSizeInBytes == Util::Engine::SHA_512_HASH_SIZE_IN_BYTES && "ERROR: For some reason, the Next Generation Cryptography API seems to be using the wrong size for an SHA-512 hash!");
#endif // _DEBUG
	}

	HashProvider::~HashProvider()
	{
		const NTSTATUS status = BCryptCloseAlgorithmProvider(mHAlgorithmProvider, 0);
		assert(Util::Win32::NT_SUCCESS(status));
	}

	BCRYPT_ALG_HANDLE HashProvider::GetAlgorithmProvider() const
	{
		return mHAlgorithmProvider;
	}

	std::size_t HashProvider::GetHashObjectSize() const
	{
		return static_cast<std::size_t>(mHashObjectSize);
	}
}