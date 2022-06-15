module;
#include <vector>
#include <span>
#include <array>
#include <cassert>
#include <Windows.h>
#include <bcrypt.h>

module Brawler.SHA512Hashing;
import :SHA512AlgorithmProvider;
import Util.General;

namespace Brawler
{
	SHA512Hasher::SHA512Hasher() :
		mHHashObject(nullptr),
		mHashObjectBackingMemory()
	{
		// Reserve the backing memory for the hash object. For some reason, the Win32 CNG requires that
		// we allocate memory for the hashing objects ourselves, and then has the audacity to ask us to
		// close the handle to the hash object manually.
		mHashObjectBackingMemory.resize(SHA512AlgorithmProvider::GetInstance().GetHashObjectSize());

		Util::General::CheckHRESULT(HRESULT_FROM_NT(BCryptCreateHash(
			SHA512AlgorithmProvider::GetInstance().GetAlgorithmProviderHandle(),
			&mHHashObject,
			reinterpret_cast<PUCHAR>(mHashObjectBackingMemory.data()),
			static_cast<ULONG>(mHashObjectBackingMemory.size()),
			nullptr,
			0,
			BCRYPT_HASH_REUSABLE_FLAG
		)));
	}

	SHA512Hasher::~SHA512Hasher()
	{
		DeleteHashObject();
	}

	SHA512Hasher::SHA512Hasher(SHA512Hasher&& rhs) noexcept :
		mHHashObject(rhs.mHHashObject),
		mHashObjectBackingMemory(std::move(rhs.mHashObjectBackingMemory))
	{
		rhs.mHHashObject = nullptr;
	}

	SHA512Hasher& SHA512Hasher::operator=(SHA512Hasher&& rhs) noexcept
	{
		DeleteHashObject();

		mHHashObject = rhs.mHHashObject;
		rhs.mHHashObject = nullptr;

		mHashObjectBackingMemory = std::move(rhs.mHashObjectBackingMemory);

		return *this;
	}

	SHA512Hash SHA512Hasher::HashData(const std::span<const std::byte> srcDataSpan) const
	{
		static constexpr std::size_t SHA_512_HASH_SIZE_BYTES = 64;
		std::array<std::byte, SHA_512_HASH_SIZE_BYTES> hashByteArr{};

		if constexpr (Util::General::IsDebugModeEnabled())
		{
			std::uint32_t hashSizeInBytes = 0;
			ULONG numBytesCopied = 0;

			Util::General::CheckHRESULT(HRESULT_FROM_NT(BCryptGetProperty(
				SHA512AlgorithmProvider::GetInstance().GetAlgorithmProviderHandle(),
				BCRYPT_HASH_LENGTH,
				reinterpret_cast<PUCHAR>(&hashSizeInBytes),
				sizeof(hashSizeInBytes),
				&numBytesCopied,
				0
			)));

			assert(hashSizeInBytes == SHA_512_HASH_SIZE_BYTES);
		}

		Util::General::CheckHRESULT(HRESULT_FROM_NT(BCryptHashData(
			mHHashObject,

			// Although the MSDN states that the function does not modify the data pointed to by pbInput, they
			// did not make the parameter a const*. Don't ask me why.
			reinterpret_cast<PUCHAR>(const_cast<std::byte*>(srcDataSpan.data())),

			static_cast<ULONG>(srcDataSpan.size_bytes()),
			0
		)));

		Util::General::CheckHRESULT(HRESULT_FROM_NT(BCryptFinishHash(
			mHHashObject,
			reinterpret_cast<PUCHAR>(hashByteArr.data()),
			static_cast<ULONG>(hashByteArr.size()),
			0
		)));

		return SHA512Hash{ std::move(hashByteArr) };
	}

	void SHA512Hasher::DeleteHashObject()
	{
		if (mHHashObject != nullptr)
		{
			// The MSDN states that we actually need to destroy the hash object *BEFORE* we clear its backing
			// memory, so we have to do it in this order.

			Util::General::CheckHRESULT(HRESULT_FROM_NT(BCryptDestroyHash(mHHashObject)));

			mHHashObject = nullptr;

			mHashObjectBackingMemory.clear();
			mHashObjectBackingMemory.shrink_to_fit();
		}
	}
}