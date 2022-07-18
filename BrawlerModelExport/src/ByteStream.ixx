module;
#include <vector>
#include <span>
#include <iostream>

export module Brawler.ByteStream;

export namespace Brawler
{
	class ByteStream;
}

namespace Brawler
{
	template <typename T>
	struct GenericTypeSolver
	{
		static constexpr bool IS_GENERIC = true;
	};

	template <typename T, std::size_t N>
	struct GenericTypeSolver<std::span<T, N>>
	{
		static constexpr bool IS_GENERIC = false;
	};

	template <>
	struct GenericTypeSolver<ByteStream>
	{
		static constexpr bool IS_GENERIC = false;
	};
}

export
{
	std::ostream& operator<<(std::ostream& lhs, Brawler::ByteStream& rhs);
	constexpr Brawler::ByteStream& operator<<(Brawler::ByteStream& lhs, Brawler::ByteStream& rhs);
	constexpr Brawler::ByteStream& operator>>(Brawler::ByteStream& lhs, Brawler::ByteStream& rhs);

	template <typename T>
		requires Brawler::GenericTypeSolver<T>::IS_GENERIC
	constexpr Brawler::ByteStream& operator<<(Brawler::ByteStream& lhs, const T& rhs);

	template <typename T, std::size_t N>
	constexpr Brawler::ByteStream& operator<<(Brawler::ByteStream& lhs, const std::span<const T, N> rhsDataSpan);
}

export namespace Brawler
{
	class ByteStream
	{
	private:
		friend std::ostream& (::operator<<)(std::ostream& lhs, ByteStream& rhs);
		friend constexpr ByteStream& (::operator<<)(ByteStream& lhs, ByteStream& rhs);
		friend constexpr ByteStream& (::operator>>)(ByteStream& lhs, ByteStream& rhs);

		template <typename T>
			requires Brawler::GenericTypeSolver<T>::IS_GENERIC
		friend constexpr ByteStream& (::operator<<)(ByteStream& lhs, const T& rhs);

		template <typename T, std::size_t N>
		friend constexpr Brawler::ByteStream& (::operator<<)(ByteStream& lhs, const std::span<const T, N> rhsDataSpan);

	public:
		constexpr ByteStream() = default;

		constexpr ByteStream(const ByteStream& rhs) = delete;
		constexpr ByteStream& operator=(const ByteStream& rhs) = delete;

		constexpr ByteStream(ByteStream&& rhs) noexcept = default;
		constexpr ByteStream& operator=(ByteStream&& rhs) noexcept = default;

		constexpr std::size_t GetByteCount() const;
		constexpr void ResetStream();

	private:
		constexpr std::span<std::byte> GetWriteableByteSpan();
		constexpr std::span<const std::byte> GetByteSpan() const;

	private:
		std::vector<std::byte> mByteArr;
		std::size_t mCurrIndex;
	};
}

// ------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	constexpr std::size_t ByteStream::GetByteCount() const
	{
		return (mCurrIndex < mByteArr.size() ? (mByteArr.size() - mCurrIndex) : 0);
	}

	constexpr void ByteStream::ResetStream()
	{
		mByteArr.clear();
		mByteArr.shrink_to_fit();

		mCurrIndex = 0;
	}

	constexpr std::span<std::byte> ByteStream::GetWriteableByteSpan()
	{
		if (mCurrIndex >= mByteArr.size()) [[unlikely]]
			return std::span<std::byte>{};

		std::span<std::byte> writeableByteSpan{ mByteArr };
		return writeableByteSpan.subspan(mCurrIndex);
	}

	constexpr std::span<const std::byte> ByteStream::GetByteSpan() const
	{
		if (mCurrIndex >= mByteArr.size()) [[unlikely]]
			return std::span<const std::byte>{};

		std::span<const std::byte> byteSpan{ mByteArr };
		return byteSpan.subspan(mCurrIndex);
	}
}

std::ostream& operator<<(std::ostream& lhs, Brawler::ByteStream& rhs)
{
	if (rhs.mCurrIndex >= rhs.mByteArr.size()) [[unlikely]]
		return lhs;

	const std::span<const std::byte> srcDataByteSpan{ rhs.GetByteSpan() };
	lhs.write(reinterpret_cast<const char*>(srcDataByteSpan.data()), srcDataByteSpan.size_bytes());

	rhs.ResetStream();

	return lhs;
}

constexpr Brawler::ByteStream& operator<<(Brawler::ByteStream& lhs, Brawler::ByteStream& rhs)
{
	const std::span<const std::byte> srcDataByteSpan{ rhs.GetByteSpan() };
	lhs.mByteArr.reserve(lhs.mByteArr.size() + srcDataByteSpan.size_bytes());

	for (const auto byte : srcDataByteSpan)
		lhs.mByteArr.push_back(byte);

	rhs.ResetStream();

	return lhs;
}

constexpr Brawler::ByteStream& operator>>(Brawler::ByteStream& lhs, Brawler::ByteStream& rhs)
{
	rhs << lhs;

	return lhs;
}

template <typename T>
	requires Brawler::GenericTypeSolver<T>::IS_GENERIC
constexpr Brawler::ByteStream& operator<<(Brawler::ByteStream& lhs, const T& rhs)
{
	lhs.mByteArr.reserve(lhs.mByteArr.size() + sizeof(rhs));
	const std::span<const std::byte> srcDataByteSpan{ std::as_bytes(std::span<const T>{&rhs, 1}) };

	for (const auto byte : srcDataByteSpan)
		lhs.mByteArr.push_back(byte);

	return lhs;
}

template <typename T, std::size_t N>
constexpr Brawler::ByteStream& operator<<(Brawler::ByteStream& lhs, const std::span<const T, N> rhsDataSpan)
{
	for (const auto& dataElement : rhsDataSpan)
		lhs << dataElement;

	return lhs;
}