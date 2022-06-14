module;
#include <memory>
#include <DxDef.h>
#include <bcrypt.h>

module Brawler.SHA512Hashing;
import Util.General;

namespace Brawler
{
	SHA512AlgorithmProvider::SHA512AlgorithmProvider() :
		mHAlgorithmProvider(nullptr),
		mHashObjectSize(0)
	{
		Util::General::CheckHRESULT(HRESULT_FROM_NT(BCryptOpenAlgorithmProvider(
			std::out_ptr(mHAlgorithmProvider),
			BCRYPT_SHA512_ALGORITHM,
			nullptr,
			BCRYPT_HASH_REUSABLE_FLAG
		)));

		ULONG numBytesCopied = 0;

		Util::General::CheckHRESULT(HRESULT_FROM_NT(BCryptGetProperty(
			mHAlgorithmProvider.get(),
			BCRYPT_OBJECT_LENGTH,
			reinterpret_cast<PUCHAR>(&mHashObjectSize),
			sizeof(mHashObjectSize),
			&numBytesCopied,
			0
		)));
	}

	SHA512AlgorithmProvider& SHA512AlgorithmProvider::GetInstance()
	{
		static SHA512AlgorithmProvider instance{};
		return instance;
	}

	BCRYPT_ALG_HANDLE SHA512AlgorithmProvider::GetAlgorithmProviderHandle() const
	{
		return mHAlgorithmProvider.get();
	}

	std::uint32_t SHA512AlgorithmProvider::GetHashObjectSize() const
	{
		return mHashObjectSize;
	}
}