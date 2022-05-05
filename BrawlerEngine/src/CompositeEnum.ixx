module;
#include <type_traits>
#include <array>

export module Brawler.CompositeEnum;
import Util.General;

export namespace Brawler
{
	// By convention, we denote the count of an enum class with a special enumeration
	// added to the end of its list: COUNT_OR_ERROR.

	template <typename T, std::size_t EnumCount = static_cast<std::size_t>(T::COUNT_OR_ERROR)>
		requires std::is_enum_v<T>
	class CompositeEnum
	{
	public:
		CompositeEnum();

		template <typename U>
			requires std::is_same_v<std::decay_t<U>, std::decay_t<T>>
		CompositeEnum(const std::decay_t<U> enumValue);

		bool IsBitSet(const T enumValue) const;

		CompositeEnum& operator&(const CompositeEnum& rhs);
		CompositeEnum& operator|(const CompositeEnum& rhs);
		CompositeEnum& operator^(const CompositeEnum& rhs);
		CompositeEnum& operator~();

		CompositeEnum& operator&=(const CompositeEnum& rhs);
		CompositeEnum& operator|=(const CompositeEnum& rhs);
		CompositeEnum& operator^=(const CompositeEnum& rhs);

	private:
		std::uint64_t& GetRelevantBitMask(const T enumValue);
		const std::uint64_t& GetRelevantBitMask(const T enumValue) const;

		std::uint64_t ConvertEnumValueToBitMask(const T enumValue) const;

	private:
		std::array<std::uint64_t, ((EnumCount / 64) + 1)> mBitMaskArr;
	};

	// --------------------------------------------------------------------------------------------------------

	template <typename T, std::size_t EnumCount>
		requires std::is_enum_v<T>
	CompositeEnum<T, EnumCount>::CompositeEnum() :
		mBitMaskArr()
	{}

	template <typename T, std::size_t EnumCount>
		requires std::is_enum_v<T>
	template <typename U>
		requires std::is_same_v<std::decay_t<U>, std::decay_t<T>>
	CompositeEnum<T, EnumCount>::CompositeEnum(const std::decay_t<U> enumValue) :
		mBitMaskArr()
	{
		GetRelevantBitMask(enumValue) |= ConvertEnumValueToBitMask(enumValue);
	}

	template <typename T, std::size_t EnumCount>
		requires std::is_enum_v<T>
	bool CompositeEnum<T, EnumCount>::IsBitSet(const T enumValue) const
	{
		return (GetRelevantBitMask(enumValue) & ConvertEnumValueToBitMask(enumValue));
	}

	template <typename T, std::size_t EnumCount>
		requires std::is_enum_v<T>
	CompositeEnum<T, EnumCount>& CompositeEnum<T, EnumCount>::operator&(const CompositeEnum& rhs)
	{
		for (std::size_t i = 0; i < mBitMaskArr.size(); ++i)
			mBitMaskArr[i] &= rhs.mBitMaskArr[i];

		return *this;
	}

	template <typename T, std::size_t EnumCount>
		requires std::is_enum_v<T>
	CompositeEnum<T, EnumCount>& CompositeEnum<T, EnumCount>::operator|(const CompositeEnum& rhs)
	{
		for (std::size_t i = 0; i < mBitMaskArr.size(); ++i)
			mBitMaskArr[i] |= rhs.mBitMaskArr[i];

		return *this;
	}

	template <typename T, std::size_t EnumCount>
		requires std::is_enum_v<T>
	CompositeEnum<T, EnumCount>& CompositeEnum<T, EnumCount>::operator^(const CompositeEnum& rhs)
	{
		for (std::size_t i = 0; i < mBitMaskArr.size(); ++i)
			mBitMaskArr[i] ^= rhs.mBitMaskArr[i];

		return *this;
	}

	template <typename T, std::size_t EnumCount>
		requires std::is_enum_v<T>
	CompositeEnum<T, EnumCount>& CompositeEnum<T, EnumCount>::operator~()
	{
		for (auto& bitMask : mBitMaskArr)
			bitMask = ~bitMask;

		return *this;
	}

	template <typename T, std::size_t EnumCount>
		requires std::is_enum_v<T>
	CompositeEnum<T, EnumCount>& CompositeEnum<T, EnumCount>::operator&=(const CompositeEnum& rhs)
	{
		*this = (*this & rhs);

		return *this;
	}

	template <typename T, std::size_t EnumCount>
		requires std::is_enum_v<T>
	CompositeEnum<T, EnumCount>& CompositeEnum<T, EnumCount>::operator|=(const CompositeEnum& rhs)
	{
		*this = (*this | rhs);

		return *this;
	}

	template <typename T, std::size_t EnumCount>
		requires std::is_enum_v<T>
	CompositeEnum<T, EnumCount>& CompositeEnum<T, EnumCount>::operator^=(const CompositeEnum& rhs)
	{
		*this = (*this ^ rhs);

		return *this;
	}

	template <typename T, std::size_t EnumCount>
		requires std::is_enum_v<T>
	std::uint64_t& CompositeEnum<T, EnumCount>::GetRelevantBitMask(const T enumValue)
	{
		return mBitMaskArr[static_cast<std::size_t>(enumValue) / 64];
	}

	template <typename T, std::size_t EnumCount>
		requires std::is_enum_v<T>
	const std::uint64_t& CompositeEnum<T, EnumCount>::GetRelevantBitMask(const T enumValue) const
	{
		return mBitMaskArr[static_cast<std::size_t>(enumValue) / 64];
	}

	template <typename T, std::size_t EnumCount>
		requires std::is_enum_v<T>
	std::uint64_t CompositeEnum<T, EnumCount>::ConvertEnumValueToBitMask(const T enumValue) const
	{
		return (static_cast<std::uint64_t>(1) << (Util::General::EnumCast(enumValue) % 64));
	}
}