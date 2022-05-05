module;
#include <bitset>
#include <type_traits>
#include <compare>
#include <vector>

export module Brawler.CompositeEnum;

namespace Brawler
{
	template <typename T>
	concept IsEnumWithDefinedSize = std::is_enum_v<T> && requires (T x)
	{
		T::COUNT_OR_ERROR;
	};

	template <typename T>
	concept IsValidEnum = IsEnumWithDefinedSize<T> && (std::to_underlying(T::COUNT_OR_ERROR) <= 65);

	template <typename EnumType>
		requires IsValidEnum<EnumType>
	class CompositeEnum;
}

export
{
	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	Brawler::CompositeEnum<EnumType> operator&(const Brawler::CompositeEnum<EnumType>& lhs, const EnumType rhs);

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	Brawler::CompositeEnum<EnumType> operator&(const EnumType lhs, const Brawler::CompositeEnum<EnumType>& rhs);

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	Brawler::CompositeEnum<EnumType> operator&(const Brawler::CompositeEnum<EnumType>& lhs, const Brawler::CompositeEnum<EnumType>& rhs);

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	Brawler::CompositeEnum<EnumType> operator|(const Brawler::CompositeEnum<EnumType>& lhs, const EnumType rhs);

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	Brawler::CompositeEnum<EnumType> operator|(const EnumType lhs, const Brawler::CompositeEnum<EnumType>& rhs);

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	Brawler::CompositeEnum<EnumType> operator|(const Brawler::CompositeEnum<EnumType>& lhs, const Brawler::CompositeEnum<EnumType>& rhs);

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	Brawler::CompositeEnum<EnumType> operator^(const Brawler::CompositeEnum<EnumType>& lhs, const EnumType rhs);

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	Brawler::CompositeEnum<EnumType> operator^(const EnumType lhs, const Brawler::CompositeEnum<EnumType>& rhs);

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	Brawler::CompositeEnum<EnumType> operator^(const Brawler::CompositeEnum<EnumType>& lhs, const Brawler::CompositeEnum<EnumType>& rhs);

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	std::strong_ordering operator<=>(const Brawler::CompositeEnum<EnumType>& lhs, const std::underlying_type_t<EnumType> rhs);

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	std::strong_ordering operator<=>(const std::underlying_type_t<EnumType> lhs, const Brawler::CompositeEnum<EnumType>& rhs);
}

export namespace Brawler
{
	template <typename EnumType>
		requires IsValidEnum<EnumType>
	class CompositeEnum
	{
	private:
		friend CompositeEnum (::operator&)(const CompositeEnum& lhs, const EnumType rhs);
		friend CompositeEnum (::operator&)(const EnumType lhs, const CompositeEnum& rhs);
		friend CompositeEnum (::operator&)(const CompositeEnum& lhs, const CompositeEnum& rhs);
		friend CompositeEnum (::operator|)(const CompositeEnum& lhs, const EnumType rhs);
		friend CompositeEnum (::operator|)(const EnumType lhs, const CompositeEnum& rhs);
		friend CompositeEnum (::operator|)(const CompositeEnum& lhs, const CompositeEnum& rhs);
		friend CompositeEnum (::operator^)(const CompositeEnum& lhs, const EnumType rhs);
		friend CompositeEnum (::operator^)(const EnumType lhs, const CompositeEnum& rhs);
		friend CompositeEnum (::operator^)(const CompositeEnum& lhs, const CompositeEnum& rhs);
		friend std::strong_ordering (::operator<=>)(const CompositeEnum& lhs, const std::underlying_type_t<EnumType> rhs);
		friend std::strong_ordering (::operator<=>)(const std::underlying_type_t<EnumType> lhs, const CompositeEnum& rhs);

	public:
		CompositeEnum() = default;
		CompositeEnum(const EnumType initialValue);

		CompositeEnum& operator&=(const EnumType rhs);
		CompositeEnum& operator&= (const CompositeEnum& rhs);

		CompositeEnum& operator|=(const EnumType rhs);
		CompositeEnum& operator|=(const CompositeEnum& rhs);

		CompositeEnum& operator^=(const EnumType rhs);
		CompositeEnum& operator^=(const CompositeEnum& rhs);

		CompositeEnum operator~() const;

		operator std::underlying_type_t<EnumType>() const;

	private:
		std::bitset<static_cast<std::size_t>(EnumType::COUNT_OR_ERROR)> mBitSet;
	};
}

// ---------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename EnumType>
		requires IsValidEnum<EnumType>
	CompositeEnum<EnumType>::CompositeEnum(const EnumType initialValue) :
		mBitSet()
	{
		mBitSet[std::to_underlying(initialValue)] = true;
	}

	template <typename EnumType>
		requires IsValidEnum<EnumType>
	CompositeEnum<EnumType>& CompositeEnum<EnumType>::operator&=(const EnumType rhs)
	{
		mBitSet &= (static_cast<std::uint64_t>(1) << std::to_underlying(rhs));

		return *this;
	}

	template <typename EnumType>
		requires IsValidEnum<EnumType>
	CompositeEnum<EnumType>& CompositeEnum<EnumType>::operator&=(const CompositeEnum<EnumType>& rhs)
	{
		mBitSet &= rhs.mBitSet;

		return *this;
	}

	template <typename EnumType>
		requires IsValidEnum<EnumType>
	CompositeEnum<EnumType>& CompositeEnum<EnumType>::operator|=(const EnumType rhs)
	{
		mBitSet |= (1 << std::to_underlying(rhs));

		return *this;
	}

	template <typename EnumType>
		requires IsValidEnum<EnumType>
	CompositeEnum<EnumType>& CompositeEnum<EnumType>::operator|=(const CompositeEnum<EnumType>& rhs)
	{
		mBitSet |= rhs.mBitSet;

		return *this;
	}

	template <typename EnumType>
		requires IsValidEnum<EnumType>
	CompositeEnum<EnumType>& CompositeEnum<EnumType>::operator^=(const EnumType rhs)
	{
		mBitSet ^= (1 << std::to_underlying(rhs));

		return *this;
	}

	template <typename EnumType>
		requires IsValidEnum<EnumType>
	CompositeEnum<EnumType>& CompositeEnum<EnumType>::operator^=(const CompositeEnum<EnumType>& rhs)
	{
		mBitSet ^= rhs.mBitSet;

		return *this;
	}

	template <typename EnumType>
		requires IsValidEnum<EnumType>
	CompositeEnum<EnumType> CompositeEnum<EnumType>::operator~() const
	{
		CompositeEnum<EnumType> invertedEnum{};
		invertedEnum.mBitSet = ~mBitSet;

		return invertedEnum;
	}

	template <typename EnumType>
		requires IsValidEnum<EnumType>
	CompositeEnum<EnumType>::operator std::underlying_type_t<EnumType>() const
	{
		return static_cast<std::underlying_type_t<EnumType>>(mBitSet.to_ullong());
	}
}

export
{
	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	Brawler::CompositeEnum<EnumType> operator&(const Brawler::CompositeEnum<EnumType>& lhs, const EnumType rhs)
	{
		Brawler::CompositeEnum<EnumType> retEnum{ lhs };
		retEnum &= rhs;

		return retEnum;
	}

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	Brawler::CompositeEnum<EnumType> operator&(const EnumType lhs, const Brawler::CompositeEnum<EnumType>& rhs)
	{
		return (rhs & lhs);
	}

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	Brawler::CompositeEnum<EnumType> operator&(const Brawler::CompositeEnum<EnumType>& lhs, const Brawler::CompositeEnum<EnumType>& rhs)
	{
		Brawler::CompositeEnum<EnumType> retEnum{};
		retEnum.mBitSet = (lhs.mBitSet & rhs.mBitSet);

		return retEnum;
	}

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	Brawler::CompositeEnum<EnumType> operator|(const Brawler::CompositeEnum<EnumType>& lhs, const EnumType rhs)
	{
		Brawler::CompositeEnum<EnumType> retEnum{ lhs };
		retEnum |= rhs;

		return retEnum;
	}

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	Brawler::CompositeEnum<EnumType> operator|(const EnumType lhs, const Brawler::CompositeEnum<EnumType>& rhs)
	{
		return (rhs | lhs);
	}

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	Brawler::CompositeEnum<EnumType> operator|(const Brawler::CompositeEnum<EnumType>& lhs, const Brawler::CompositeEnum<EnumType>& rhs)
	{
		Brawler::CompositeEnum<EnumType> retEnum{};
		retEnum.mBitSet = (lhs.mBitSet | rhs.mBitSet);

		return retEnum;
	}

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	Brawler::CompositeEnum<EnumType> operator^(const Brawler::CompositeEnum<EnumType>& lhs, const EnumType rhs)
	{
		Brawler::CompositeEnum<EnumType> retEnum{ lhs };
		retEnum ^= rhs;

		return retEnum;
	}

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	Brawler::CompositeEnum<EnumType> operator^(const EnumType lhs, const Brawler::CompositeEnum<EnumType>& rhs)
	{
		return (rhs ^ lhs);
	}

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	Brawler::CompositeEnum<EnumType> operator^(const Brawler::CompositeEnum<EnumType>& lhs, const Brawler::CompositeEnum<EnumType>& rhs)
	{
		Brawler::CompositeEnum<EnumType> retEnum{};
		retEnum.mBitSet = (lhs.mBitSet ^ rhs.mBitSet);

		return retEnum;
	}

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	std::strong_ordering operator<=>(const Brawler::CompositeEnum<EnumType>& lhs, const std::underlying_type_t<EnumType> rhs)
	{
		return (lhs.mBitSet.to_ullong() <=> rhs);
	}

	template <typename EnumType>
		requires Brawler::IsValidEnum<EnumType>
	std::strong_ordering operator<=>(const std::underlying_type_t<EnumType> lhs, const Brawler::CompositeEnum<EnumType>& rhs)
	{
		return (lhs <=> rhs.mBitSet.to_ullong());
	}
}