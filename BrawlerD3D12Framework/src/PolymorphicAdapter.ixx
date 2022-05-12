module;
#include <variant>
#include <optional>
#include <cassert>
#include <stdexcept>

export module Brawler.PolymorphicAdapter;
import Brawler.PolymorphismInfo;

namespace Brawler
{
	template <typename BaseType>
	class PolymorphicAdapterIMPL
	{};

	template <template<typename...> typename BaseType, typename DerivedType, typename... OtherParams>
	concept IsDerivedType = std::derived_from<DerivedType, BaseType<DerivedType, OtherParams...>>;

	template <template<typename...> typename BaseType, typename DummyType, typename... OtherParams>
	class PolymorphicAdapterIMPL<BaseType<DummyType, OtherParams...>>
	{
	private:
		using PolymorphismInfo_ = PolymorphismInfo<BaseType<void, OtherParams...>>;
		using FirstType = PolymorphismInfo_::template DerivedType<static_cast<typename PolymorphismInfo_::EnumType>(0)>;

	public:
		constexpr PolymorphicAdapterIMPL() = default;

		constexpr PolymorphicAdapterIMPL(const PolymorphicAdapterIMPL& rhs) = default;
		constexpr PolymorphicAdapterIMPL& operator=(const PolymorphicAdapterIMPL& rhs) = default;

		constexpr PolymorphicAdapterIMPL(PolymorphicAdapterIMPL&& rhs) noexcept = default;
		constexpr PolymorphicAdapterIMPL& operator=(PolymorphicAdapterIMPL&& rhs) noexcept = default;

		template <typename DerivedType>
			requires IsDerivedType<BaseType, DerivedType, OtherParams...>
		constexpr PolymorphicAdapterIMPL(DerivedType&& derivedValue);

		template <typename DerivedType>
			requires IsDerivedType<BaseType, DerivedType, OtherParams...>
		constexpr PolymorphicAdapterIMPL& operator=(DerivedType&& derivedValue);

		template <typename Callback>
			requires requires (Callback callback, typename PolymorphicAdapterIMPL<BaseType<DummyType, OtherParams...>>::FirstType& value)
		{
			callback(value);
		}
		constexpr auto AccessData(const Callback& callback);

		template <typename Callback>
			requires requires (Callback callback, typename PolymorphicAdapterIMPL<BaseType<DummyType, OtherParams...>>::FirstType& value)
		{
			callback(value);
		}
		constexpr auto AccessData(const Callback& callback) const;

	private:
		template <typename Callback, PolymorphismInfo_::EnumType CurrEnum>
		constexpr auto AccessDataIMPL(const Callback& callback);

		template <typename Callback, PolymorphismInfo_::EnumType CurrEnum>
		constexpr auto AccessDataIMPL(const Callback& callback) const;

	private:
		PolymorphismInfo_::VariantType mDataVariant;
	};
}

// -------------------------------------------------------------------------------------------

namespace Brawler
{
	template <template<typename...> typename BaseType, typename DummyType, typename... OtherParams>
	template <typename DerivedType>
		requires IsDerivedType<BaseType, DerivedType, OtherParams...>
	constexpr PolymorphicAdapterIMPL<BaseType<DummyType, OtherParams...>>::PolymorphicAdapterIMPL(DerivedType&& derivedValue) :
		mDataVariant(std::forward<DerivedType>(derivedValue))
	{}

	template <template<typename...> typename BaseType, typename DummyType, typename... OtherParams>
	template <typename DerivedType>
		requires IsDerivedType<BaseType, DerivedType, OtherParams...>
	constexpr PolymorphicAdapterIMPL<BaseType<DummyType, OtherParams...>>& PolymorphicAdapterIMPL<BaseType<DummyType, OtherParams...>>::operator=(DerivedType&& derivedValue)
	{
		mDataVariant = std::forward<DerivedType>(derivedValue);

		return *this;
	}

	template <template<typename...> typename BaseType, typename DummyType, typename... OtherParams>
	template <typename Callback>
		requires requires (Callback callback, typename PolymorphicAdapterIMPL<BaseType<DummyType, OtherParams...>>::FirstType& value)
	{
		callback(value);
	}
	constexpr auto PolymorphicAdapterIMPL<BaseType<DummyType, OtherParams...>>::AccessData(const Callback& callback)
	{
		assert(mDataVariant.index() != 0 && "ERROR: An attempt was made to access a PolymorphicAdapter before it was ever assigned a type!");
		return AccessDataIMPL<Callback, static_cast<PolymorphismInfo<BaseType<void, OtherParams...>>::EnumType>(0)>(callback);
	}

	template <template<typename...> typename BaseType, typename DummyType, typename... OtherParams>
	template <typename Callback>
		requires requires (Callback callback, typename PolymorphicAdapterIMPL<BaseType<DummyType, OtherParams...>>::FirstType& value)
	{
		callback(value);
	}
	constexpr auto PolymorphicAdapterIMPL<BaseType<DummyType, OtherParams...>>::AccessData(const Callback& callback) const
	{
		assert(mDataVariant.index() != 0 && "ERROR: An attempt was made to access a PolymorphicAdapter before it was ever assigned a type!");
		return AccessDataIMPL<Callback, static_cast<PolymorphismInfo<BaseType<void, OtherParams...>>::EnumType>(0)>(callback);
	}

	template <template<typename...> typename BaseType, typename DummyType, typename... OtherParams>
	template <typename Callback, PolymorphismInfo<BaseType<void, OtherParams...>>::EnumType CurrEnum>
	constexpr auto PolymorphicAdapterIMPL<BaseType<DummyType, OtherParams...>>::AccessDataIMPL(const Callback& callback)
	{
		using EnumType = typename PolymorphismInfo<BaseType<DummyType, OtherParams...>>::EnumType;

		assert(CurrEnum != EnumType::COUNT_OR_ERROR);

		if constexpr (CurrEnum != EnumType::COUNT_OR_ERROR)
		{
			if ((mDataVariant.index() - 1) == std::to_underlying(CurrEnum))
				return callback(std::get<std::to_underlying(CurrEnum) + 1>(mDataVariant));
			else
				return AccessDataIMPL<Callback, static_cast<EnumType>(std::to_underlying(CurrEnum) + 1)>(callback);
		}
		else
		{
			std::unreachable();
			throw std::runtime_error{ "ERROR: Uh... How did we get here?" };

			return callback(std::get<1>(mDataVariant));
		}
	}

	template <template<typename...> typename BaseType, typename DummyType, typename... OtherParams>
	template <typename Callback, PolymorphismInfo<BaseType<void, OtherParams...>>::EnumType CurrEnum>
	constexpr auto PolymorphicAdapterIMPL<BaseType<DummyType, OtherParams...>>::AccessDataIMPL(const Callback& callback) const
	{
		using EnumType = typename PolymorphismInfo<BaseType<DummyType, OtherParams...>>::EnumType;
		
		assert(CurrEnum != EnumType::COUNT_OR_ERROR);

		if constexpr (CurrEnum != EnumType::COUNT_OR_ERROR)
		{
			if ((mDataVariant.index() - 1) == std::to_underlying(CurrEnum))
				return callback(std::get<std::to_underlying(CurrEnum) + 1>(mDataVariant));
			else
				return AccessDataIMPL<Callback, static_cast<EnumType>(std::to_underlying(CurrEnum) + 1)>(callback);
		}
		else
		{
			std::unreachable();
			throw std::runtime_error{ "ERROR: Uh... How did we get here?" };

			return callback(std::get<1>(mDataVariant));
		}
	}
}

export namespace Brawler
{
	template <template<typename...> typename BaseType, typename... OtherParams>
	using PolymorphicAdapter = PolymorphicAdapterIMPL<BaseType<void, OtherParams...>>;
}