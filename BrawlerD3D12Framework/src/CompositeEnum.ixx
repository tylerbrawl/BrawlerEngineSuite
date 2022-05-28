module;
#include <bitset>
#include <type_traits>
#include <compare>
#include <vector>
#include <array>

export module Brawler.CompositeEnum;
import Util.Math;

namespace Brawler
{
	template <typename T>
	concept IsEnumWithDefinedSize = std::is_enum_v<T> && requires (T x)
	{
		T::COUNT_OR_ERROR;
	};

	template <typename T>
	concept IsValidEnum = IsEnumWithDefinedSize<T>;

	template <typename EnumType>
		requires IsValidEnum<EnumType>
	class CompositeEnum;

	template <typename EnumType>
		requires IsValidEnum<EnumType>
	consteval std::size_t GetBitSetArraySize()
	{
		const std::underlying_type_t<EnumType> enumCount = std::to_underlying(EnumType::COUNT_OR_ERROR);

		if (enumCount == 0)
			return 0;

		return ((enumCount % 64) == 0 ? (enumCount / 64) : (enumCount / 64) + 1);
	}
}

export
{
	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	constexpr Brawler::CompositeEnum<EnumType> operator&(const Brawler::CompositeEnum<EnumType>& lhs, const EnumType rhs);

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	constexpr Brawler::CompositeEnum<EnumType> operator&(const EnumType lhs, const Brawler::CompositeEnum<EnumType>& rhs);

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	constexpr Brawler::CompositeEnum<EnumType> operator&(const Brawler::CompositeEnum<EnumType>& lhs, const Brawler::CompositeEnum<EnumType>& rhs);

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	constexpr Brawler::CompositeEnum<EnumType> operator&(const EnumType lhs, const EnumType rhs);

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	constexpr Brawler::CompositeEnum<EnumType> operator|(const Brawler::CompositeEnum<EnumType>& lhs, const EnumType rhs);

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	constexpr Brawler::CompositeEnum<EnumType> operator|(const EnumType lhs, const Brawler::CompositeEnum<EnumType>& rhs);

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	constexpr Brawler::CompositeEnum<EnumType> operator|(const Brawler::CompositeEnum<EnumType>& lhs, const Brawler::CompositeEnum<EnumType>& rhs);

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	constexpr Brawler::CompositeEnum<EnumType> operator|(const EnumType lhs, const EnumType rhs);

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	constexpr Brawler::CompositeEnum<EnumType> operator^(const Brawler::CompositeEnum<EnumType>& lhs, const EnumType rhs);

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	constexpr Brawler::CompositeEnum<EnumType> operator^(const EnumType lhs, const Brawler::CompositeEnum<EnumType>& rhs);

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	constexpr Brawler::CompositeEnum<EnumType> operator^(const Brawler::CompositeEnum<EnumType>& lhs, const Brawler::CompositeEnum<EnumType>& rhs);

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	constexpr Brawler::CompositeEnum<EnumType> operator^(const EnumType lhs, const EnumType rhs);
}

export namespace Brawler
{
	template <typename EnumType>
		requires IsValidEnum<EnumType>
	class CompositeEnum
	{
	private:
		friend constexpr CompositeEnum (::operator&)(const CompositeEnum& lhs, const EnumType rhs);
		friend constexpr CompositeEnum (::operator&)(const EnumType lhs, const CompositeEnum& rhs);
		friend constexpr CompositeEnum (::operator&)(const CompositeEnum& lhs, const CompositeEnum& rhs);
		friend constexpr CompositeEnum (::operator|)(const CompositeEnum& lhs, const EnumType rhs);
		friend constexpr CompositeEnum (::operator|)(const EnumType lhs, const CompositeEnum& rhs);
		friend constexpr CompositeEnum (::operator|)(const CompositeEnum& lhs, const CompositeEnum& rhs);
		friend constexpr CompositeEnum (::operator^)(const CompositeEnum& lhs, const EnumType rhs);
		friend constexpr CompositeEnum (::operator^)(const EnumType lhs, const CompositeEnum& rhs);
		friend constexpr CompositeEnum (::operator^)(const CompositeEnum& lhs, const CompositeEnum& rhs);

	public:
		constexpr CompositeEnum() = default;
		constexpr CompositeEnum(const EnumType initialValue);

		constexpr CompositeEnum& operator&=(const EnumType rhs);
		constexpr CompositeEnum& operator&= (const CompositeEnum& rhs);

		constexpr CompositeEnum& operator|=(const EnumType rhs);
		constexpr CompositeEnum& operator|=(const CompositeEnum& rhs);

		constexpr CompositeEnum& operator^=(const EnumType rhs);
		constexpr CompositeEnum& operator^=(const CompositeEnum& rhs);

		constexpr CompositeEnum operator~() const;

		constexpr bool operator==(const EnumType rhs) const;
		constexpr bool operator==(const CompositeEnum& rhs) const;

		constexpr std::uint32_t CountOneBits() const;

		constexpr bool ContainsFlags(const CompositeEnum& rhs) const;

	private:
		constexpr std::size_t GetBitSetArrayIndex(const EnumType enumValue) const;

	private:
		std::array<std::uint64_t, GetBitSetArraySize<EnumType>()> mBitSetArr;
	};
}

// ---------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename EnumType>
		requires IsValidEnum<EnumType>
	constexpr CompositeEnum<EnumType>::CompositeEnum(const EnumType initialValue) :
		mBitSetArr()
	{
		mBitSetArr[GetBitSetArrayIndex(initialValue)] |= (static_cast<std::uint64_t>(1) << std::to_underlying(initialValue));
	}

	template <typename EnumType>
		requires IsValidEnum<EnumType>
	constexpr CompositeEnum<EnumType>& CompositeEnum<EnumType>::operator&=(const EnumType rhs)
	{
		*this &= CompositeEnum<EnumType>{ rhs };
		
		return *this;
	}

	template <typename EnumType>
		requires IsValidEnum<EnumType>
	constexpr CompositeEnum<EnumType>& CompositeEnum<EnumType>::operator&=(const CompositeEnum<EnumType>& rhs)
	{
		for (std::size_t i = 0; i < mBitSetArr.size(); ++i)
			mBitSetArr[i] &= rhs.mBitSetArr[i];

		return *this;
	}

	template <typename EnumType>
		requires IsValidEnum<EnumType>
	constexpr CompositeEnum<EnumType>& CompositeEnum<EnumType>::operator|=(const EnumType rhs)
	{
		*this |= CompositeEnum<EnumType>{ rhs };

		return *this;
	}

	template <typename EnumType>
		requires IsValidEnum<EnumType>
	constexpr CompositeEnum<EnumType>& CompositeEnum<EnumType>::operator|=(const CompositeEnum<EnumType>& rhs)
	{
		for (std::size_t i = 0; i < mBitSetArr.size(); ++i)
			mBitSetArr[i] |= rhs.mBitSetArr[i];

		return *this;
	}

	template <typename EnumType>
		requires IsValidEnum<EnumType>
	constexpr CompositeEnum<EnumType>& CompositeEnum<EnumType>::operator^=(const EnumType rhs)
	{
		mBitSetArr ^= (static_cast<std::uint64_t>(1) << std::to_underlying(rhs));

		return *this;
	}

	template <typename EnumType>
		requires IsValidEnum<EnumType>
	constexpr CompositeEnum<EnumType>& CompositeEnum<EnumType>::operator^=(const CompositeEnum<EnumType>& rhs)
	{
		for (std::size_t i = 0; i < mBitSetArr.size(); ++i)
			mBitSetArr[i] ^= rhs.mBitSetArr[i];

		return *this;
	}

	template <typename EnumType>
		requires IsValidEnum<EnumType>
	constexpr CompositeEnum<EnumType> CompositeEnum<EnumType>::operator~() const
	{
		CompositeEnum<EnumType> invertedEnum{};

		for (std::size_t i = 0; i < mBitSetArr.size(); ++i)
			invertedEnum.mBitSetArr[i] = ~(mBitSetArr[i]);

		return invertedEnum;
	}

	template <typename EnumType>
		requires IsValidEnum<EnumType>
	constexpr bool CompositeEnum<EnumType>::operator==(const EnumType rhs) const
	{
		return (*this == CompositeEnum<EnumType>{ rhs });
	}

	template <typename EnumType>
		requires IsValidEnum<EnumType>
	constexpr bool CompositeEnum<EnumType>::operator==(const CompositeEnum<EnumType>& rhs) const
	{
		for (std::size_t i = 0; i < mBitSetArr.size(); ++i)
		{
			if (mBitSetArr[i] != rhs.mBitSetArr[i])
				return false;
		}

		return true;
	}

	template <typename EnumType>
		requires IsValidEnum<EnumType>
	constexpr std::uint32_t CompositeEnum<EnumType>::CountOneBits() const
	{
		std::size_t onesCount = 0;

		for (const auto& bitSet : mBitSetArr)
			onesCount += Util::Math::CountOneBits(bitSet);

		return static_cast<std::uint32_t>(onesCount);
	}

	template <typename EnumType>
		requires IsValidEnum<EnumType>
	constexpr bool CompositeEnum<EnumType>::ContainsFlags(const CompositeEnum& rhs) const
	{
		const CompositeEnum<EnumType> combinedEnum{ *this & rhs };

		// We know that *this contains all of the flags in rhs iff (*this & rhs) has just as
		// many one bits as rhs does.
		return (combinedEnum.CountOneBits() == rhs.CountOneBits());
	}

	template <typename EnumType>
		requires IsValidEnum<EnumType>
	constexpr std::size_t CompositeEnum<EnumType>::GetBitSetArrayIndex(const EnumType enumValue) const
	{
		return (std::to_underlying(enumValue) / 64);
	}
}

export
{
	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	constexpr Brawler::CompositeEnum<EnumType> operator&(const Brawler::CompositeEnum<EnumType>& lhs, const EnumType rhs)
	{
		return (lhs & Brawler::CompositeEnum<EnumType>{ rhs });
	}

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	constexpr Brawler::CompositeEnum<EnumType> operator&(const EnumType lhs, const Brawler::CompositeEnum<EnumType>& rhs)
	{
		return (rhs & lhs);
	}

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	constexpr Brawler::CompositeEnum<EnumType> operator&(const Brawler::CompositeEnum<EnumType>& lhs, const Brawler::CompositeEnum<EnumType>& rhs)
	{
		Brawler::CompositeEnum<EnumType> retEnum{ lhs };
		retEnum &= rhs;

		return retEnum;
	}

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	constexpr Brawler::CompositeEnum<EnumType> operator&(const EnumType lhs, const EnumType rhs)
	{
		return (Brawler::CompositeEnum<EnumType>{ lhs } & rhs);
	}

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	constexpr Brawler::CompositeEnum<EnumType> operator|(const Brawler::CompositeEnum<EnumType>& lhs, const EnumType rhs)
	{
		return (lhs | Brawler::CompositeEnum<EnumType>{ rhs });
	}

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	constexpr Brawler::CompositeEnum<EnumType> operator|(const EnumType lhs, const Brawler::CompositeEnum<EnumType>& rhs)
	{
		return (rhs | lhs);
	}

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	constexpr Brawler::CompositeEnum<EnumType> operator|(const Brawler::CompositeEnum<EnumType>& lhs, const Brawler::CompositeEnum<EnumType>& rhs)
	{
		Brawler::CompositeEnum<EnumType> retEnum{ lhs };
		retEnum |= rhs;

		return retEnum;
	}

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	constexpr Brawler::CompositeEnum<EnumType> operator|(const EnumType lhs, const EnumType rhs)
	{
		return (Brawler::CompositeEnum<EnumType>{ lhs } | rhs);
	}

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	constexpr Brawler::CompositeEnum<EnumType> operator^(const Brawler::CompositeEnum<EnumType>& lhs, const EnumType rhs)
	{
		return (lhs ^ Brawler::CompositeEnum<EnumType>{ rhs });
	}

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	constexpr Brawler::CompositeEnum<EnumType> operator^(const EnumType lhs, const Brawler::CompositeEnum<EnumType>& rhs)
	{
		return (rhs ^ lhs);
	}

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	constexpr Brawler::CompositeEnum<EnumType> operator^(const Brawler::CompositeEnum<EnumType>& lhs, const Brawler::CompositeEnum<EnumType>& rhs)
	{
		Brawler::CompositeEnum<EnumType> retEnum{ lhs };
		retEnum ^= rhs;

		return retEnum;
	}

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	constexpr Brawler::CompositeEnum<EnumType> operator^(const EnumType lhs, const EnumType rhs)
	{
		return (Brawler::CompositeEnum<EnumType>{ lhs } ^ rhs);
	}
}