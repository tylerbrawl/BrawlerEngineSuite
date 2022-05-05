module;
#include <type_traits>
#include <utility>

export module Brawler.ConvenienceTypes;

export namespace Brawler
{
	template <typename RetType, typename... Args>
	using FunctionPtr = RetType(*)(Args...);

	template <typename EnumType, EnumType... SequenceValues>
		requires std::is_enum_v<EnumType>
	using EnumSequence = std::integer_sequence<std::underlying_type_t<EnumType>, std::to_underlying(SequenceValues)...>;
}