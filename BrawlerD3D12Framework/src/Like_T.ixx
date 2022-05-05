module;
#include <type_traits>

export module Brawler.Like_T;

namespace Brawler
{
	template <typename From, typename To>
	struct Like
	{
		using Type = std::decay_t<To>;
	};

	template <typename From, typename To>
	struct Like<const From, To>
	{
		using Type = const std::decay_t<To>;
	};

	template <typename From, typename To>
	struct Like<volatile From, To>
	{
		using Type = volatile std::decay_t<To>;
	};

	template <typename From, typename To>
	struct Like<const volatile From, To>
	{
		using Type = const volatile std::decay_t<To>;
	};

	template <typename From, typename To>
	struct Like<From&, To>
	{
		using Type = Like<From, To>::Type&;
	};

	template <typename From, typename To>
	struct Like<From&&, To>
	{
		using Type = Like<From, To>::Type&&;
	};
}

export namespace Brawler
{
	template <typename From, typename To>
	using Like_T = Like<From, To>::Type;

	template <typename T, typename U>
	constexpr auto ForwardLike(std::remove_reference_t<U>& val)
	{
		return std::forward<Like_T<T, decltype(val)>>(val);
	}

	template <typename T, typename U>
	constexpr auto ForwardLike(std::remove_reference_t<U>&& val)
	{
		return std::forward<Like_T<T, decltype(val)>>(val);
	}
}