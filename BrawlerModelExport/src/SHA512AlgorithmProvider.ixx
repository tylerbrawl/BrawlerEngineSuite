module;
#include <cassert>
#include <memory>
#include <DxDef.h>
#include <bcrypt.h>

export module Brawler.SHA512Hashing:SHA512AlgorithmProvider;

namespace Brawler
{
	struct AlgorithmProviderDeleter
	{
		void operator()(const BCRYPT_ALG_HANDLE hAlgorithm) const
		{
			if (hAlgorithm != nullptr) [[likely]]
			{
				const HRESULT hr = HRESULT_FROM_NT(BCryptCloseAlgorithmProvider(hAlgorithm, 0));
				assert(SUCCEEDED(hr) && "ERROR: BCryptCloseAlgorithmProvider() failed to close an algorithm provider!");
			}
		}
	};
}

namespace Brawler
{
	using SafeBCryptAlgorithmHandle = std::unique_ptr<std::remove_pointer_t<BCRYPT_ALG_HANDLE>, AlgorithmProviderDeleter>;
}

export namespace Brawler
{
	class SHA512AlgorithmProvider final
	{
	private:
		SHA512AlgorithmProvider();

	public:
		~SHA512AlgorithmProvider() = default;

		SHA512AlgorithmProvider(const SHA512AlgorithmProvider& rhs) = delete;
		SHA512AlgorithmProvider& operator=(const SHA512AlgorithmProvider& rhs) = delete;

		SHA512AlgorithmProvider(SHA512AlgorithmProvider&& rhs) noexcept = delete;
		SHA512AlgorithmProvider& operator=(SHA512AlgorithmProvider&& rhs) noexcept = delete;

		static SHA512AlgorithmProvider& GetInstance();

		BCRYPT_ALG_HANDLE GetAlgorithmProviderHandle() const;
		std::uint32_t GetHashObjectSize() const;

	private:
		SafeBCryptAlgorithmHandle mHAlgorithmProvider;

		// NOTE: The CNG API fills this value using a pointer to a DWORD (std::uint32_t), so
		// we don't want to make this a std::size_t.
		std::uint32_t mHashObjectSize;
	};
}