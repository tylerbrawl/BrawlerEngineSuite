module;
#include <tuple>
#include <variant>
#include <optional>
#include <cassert>

export module Brawler.PolymorphismInfo;

export namespace Brawler
{
	template <typename T>
	struct PolymorphismInfo
	{
		static_assert(sizeof(T) != sizeof(T), "ERROR: An attempt was made to create a Brawler::PolymorphicAdapter for a type which was never given an explicit Brawler::PolymorphismInfo instantiation! (Did you forget to (export) import the proper polymorphism traits module? Also, module interface units which import polymorphism trait modules *MUST* also export them.)");
	};

	template <typename EnumType, typename ErrorType>
	struct PolymorphismInfoInstantiation
	{};

	template <typename EnumType_, typename... TupleTypes>
		requires requires (EnumType_ x)
	{
		EnumType_::COUNT_OR_ERROR;
	}
	struct PolymorphismInfoInstantiation<EnumType_, std::tuple<TupleTypes...>>
	{
	private:
		using TupleType = std::tuple<std::decay_t<TupleTypes>...>;

	public:
		using EnumType = EnumType_;
		using VariantType = std::variant<std::monostate, std::decay_t<TupleTypes>...>;

	private:
		template <typename DesiredType, std::size_t CurrIndex>
		static consteval std::optional<EnumType> GetEnumerationValueIMPL()
		{
			if constexpr (CurrIndex == std::to_underlying(EnumType::COUNT_OR_ERROR))
				return std::optional<EnumType>{};

			else if constexpr (std::is_same_v<DesiredType, std::tuple_element_t<CurrIndex, TupleType>>)
				return std::optional<EnumType>{ static_cast<EnumType>(CurrIndex) };

			else
				return GetEnumerationValueIMPL<DesiredType, (CurrIndex + 1)>();
		}

	public:
		template <typename DerivedType>
		static consteval EnumType GetEnumerationValue()
		{
			std::optional<EnumType> enumValue{ GetEnumerationValueIMPL<std::decay_t<DerivedType>, 0>() };
			assert(enumValue.has_value() && "ERROR: PolymorphismInfoInstantiation::GetEnumerationValue() was called for an invalid type!");

			return *enumValue;
		}

		template <EnumType EnumValue>
		using DerivedType = std::tuple_element_t<std::to_underlying(EnumValue), TupleType>;
	};
}