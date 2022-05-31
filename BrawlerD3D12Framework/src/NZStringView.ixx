module;
#include <string>
#include <string_view>
#include <cassert>

export module Brawler.NZStringView;

namespace Brawler
{
	template <typename CharT, typename Traits>
	class BasicNZStringView;

	template <typename CharT, typename Traits>
	constexpr bool operator==(const BasicNZStringView<CharT, Traits> lhs, const BasicNZStringView<CharT, Traits> rhs);

	template <typename Traits>
	concept HasDefinedComparisonCategory = requires ()
	{
		typename Traits::comparison_category;
	};

	template <typename T>
	struct ComparisonTypeSolver
	{};

	template <typename Traits>
		requires HasDefinedComparisonCategory<Traits>
	struct ComparisonTypeSolver<Traits>
	{
		using ComparisonType = typename Traits::comparison_category;
	};

	template <typename Traits>
		requires !HasDefinedComparisonCategory<Traits>
	struct ComparisonTypeSolver<Traits>
	{
		using ComparisonType = std::weak_ordering;
	};

	template <typename CharT, typename Traits>
	constexpr typename ComparisonTypeSolver<Traits>::ComparisonType operator<=>(const BasicNZStringView<CharT, Traits> lhs, const BasicNZStringView<CharT, Traits> rhs);

	template <typename CharT, typename Traits = std::char_traits<CharT>>
	class BasicNZStringView final : private std::basic_string_view<CharT, Traits>
	{
	private:
		friend constexpr bool operator==(const BasicNZStringView lhs, const BasicNZStringView rhs);
		friend constexpr typename ComparisonTypeSolver<Traits>::ComparisonType operator<=>(const BasicNZStringView lhs, const BasicNZStringView rhs);

	public:
		constexpr BasicNZStringView() = default;

		constexpr BasicNZStringView(const BasicNZStringView& rhs) = default;
		constexpr BasicNZStringView& operator=(const BasicNZStringView& rhs) = default;

		constexpr BasicNZStringView(BasicNZStringView&& rhs) noexcept = default;
		constexpr BasicNZStringView& operator=(BasicNZStringView&& rhs) noexcept = default;

		// Use a consteval constructor for the C-string variant to (try to)
		// ensure that we are getting a string literal, since we know that those are
		// always null-terminated.
		consteval BasicNZStringView(const CharT* const cStr);

		template <typename Allocator>
		constexpr BasicNZStringView(const std::basic_string<CharT, Traits, Allocator>& str);

		template <typename T>
			requires std::is_same_v<std::decay_t<T>, std::basic_string_view<CharT, Traits>>
		constexpr BasicNZStringView(T strView) = delete;

		constexpr const CharT* C_Str() const;
		
		constexpr std::size_t GetSize() const;
		constexpr bool Empty() const;

		constexpr const CharT& operator[](const std::size_t index) const;
	};
}

// -----------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename CharT, typename Traits>
	consteval BasicNZStringView<CharT, Traits>::BasicNZStringView(const CharT* const cStr) :
		std::basic_string_view<CharT, Traits>(cStr)
	{}

	template <typename CharT, typename Traits>
	template <typename Allocator>
	constexpr BasicNZStringView<CharT, Traits>::BasicNZStringView(const std::basic_string<CharT, Traits, Allocator>& str) :
		std::basic_string_view<CharT, Traits>(str)
	{}

	template <typename CharT, typename Traits>
	constexpr const CharT* BasicNZStringView<CharT, Traits>::C_Str() const
	{
		return std::basic_string_view<CharT, Traits>::data();
	}

	template <typename CharT, typename Traits>
	constexpr std::size_t BasicNZStringView<CharT, Traits>::GetSize() const
	{
		return std::basic_string_view<CharT, Traits>::size();
	}

	template <typename CharT, typename Traits>
	constexpr bool BasicNZStringView<CharT, Traits>::Empty() const
	{
		return std::basic_string_view<CharT, Traits>::empty();
	}

	template <typename CharT, typename Traits>
	constexpr const CharT& BasicNZStringView<CharT, Traits>::operator[](const std::size_t index) const
	{
		assert(index < GetSize() && "ERROR: An out-of-bounds index was specified in a call to BasicNZStringView::operator[]()!");
		return std::basic_string_view<CharT, Traits>::operator[](index);
	}

	template <typename CharT, typename Traits>
	constexpr bool operator==(const BasicNZStringView<CharT, Traits> lhs, const BasicNZStringView<CharT, Traits> rhs)
	{
		return *(static_cast<std::basic_string_view<CharT, Traits>*>(std::addressof(lhs))) == *(static_cast<std::basic_string_view<CharT, Traits>*>(std::addressof(rhs)));
	}

	template <typename CharT, typename Traits>
	constexpr typename ComparisonTypeSolver<Traits>::ComparisonType operator<=>(const BasicNZStringView<CharT, Traits> lhs, const BasicNZStringView<CharT, Traits> rhs)
	{
		return *(static_cast<std::basic_string_view<CharT, Traits>*>(std::addressof(lhs))) <=> *(static_cast<std::basic_string_view<CharT, Traits>*>(std::addressof(rhs)));
	}
}

export namespace Brawler
{
	using NZStringView = BasicNZStringView<char, std::char_traits<char>>;
	using NZWStringView = BasicNZStringView<wchar_t, std::char_traits<wchar_t>>;
}