module;

export module Brawler.TypeTraits;

namespace Brawler
{
	template <typename T>
	struct StaticFailIMPL
	{
		static constexpr bool VALUE = false;
	};
}

export namespace Brawler
{
	template <typename T>
	consteval bool StaticFail()
	{
		return StaticFailIMPL<T>::VALUE;
	}

	template <auto Val>
	consteval bool StaticFail()
	{
		return StaticFailIMPL<decltype(Val)>::VALUE;
	}
}