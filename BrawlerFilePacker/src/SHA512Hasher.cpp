module;
#include <vector>
#include <cassert>
#include <span>
#include "Win32Def.h"

module Brawler.SHA512Hasher;
import Util.Win32;
import Util.Engine;
import Brawler.HashProvider;
import Brawler.SHA512Hash;

namespace Brawler
{
	SHA512Hasher::SHA512Hasher() :
		mHHashObject(nullptr),
		mHashObjectBuffer()
	{}

	SHA512Hasher::~SHA512Hasher()
	{
		DeleteHashObject();
	}

	SHA512Hasher::SHA512Hasher(SHA512Hasher&& rhs) noexcept :
		mHHashObject(rhs.mHHashObject),
		mHashObjectBuffer(std::move(rhs.mHashObjectBuffer))
	{
		rhs.mHHashObject = nullptr;
	}

	SHA512Hasher& SHA512Hasher::operator=(SHA512Hasher&& rhs) noexcept
	{
		DeleteHashObject();
		
		mHHashObject = rhs.mHHashObject;
		rhs.mHHashObject = nullptr;

		mHashObjectBuffer = std::move(rhs.mHashObjectBuffer);

		return *this;
	}

	void SHA512Hasher::Initialize()
	{
		// Reserve the necessary space for the hash object and value.
		mHashObjectBuffer.resize(Util::Engine::GetHashProvider().GetHashObjectSize());
		
		const BCRYPT_ALG_HANDLE hAlgorithmProvider = Util::Engine::GetHashProvider().GetAlgorithmProvider();

		const NTSTATUS status = BCryptCreateHash(
			hAlgorithmProvider,
			&mHHashObject,
			mHashObjectBuffer.data(),
			static_cast<std::uint32_t>(mHashObjectBuffer.size()),
			nullptr,
			0,
			BCRYPT_HASH_REUSABLE_FLAG
		);
		assert(Util::Win32::NT_SUCCESS(status));
	}

	SHA512Hash SHA512Hasher::CreateSHA512Hash(const std::span<std::uint8_t> byteArr) const
	{
		NTSTATUS status = BCryptHashData(
			mHHashObject,
			byteArr.data(),
			static_cast<std::uint32_t>(byteArr.size_bytes()),
			0
		);
		assert(Util::Win32::NT_SUCCESS(status));

		std::array<std::uint8_t, Util::Engine::SHA_512_HASH_SIZE_IN_BYTES> hashValueBuffer{};
		status = BCryptFinishHash(
			mHHashObject,
			hashValueBuffer.data(),
			static_cast<std::uint32_t>(Util::Engine::SHA_512_HASH_SIZE_IN_BYTES),
			0
		);

		return SHA512Hash{ std::move(hashValueBuffer) };
	}

	void SHA512Hasher::DeleteHashObject()
	{
		if (mHHashObject != nullptr)
		{
			const NTSTATUS status = BCryptDestroyHash(mHHashObject);
			assert(Util::Win32::NT_SUCCESS(status));

			mHHashObject = nullptr;
		}
	}
}