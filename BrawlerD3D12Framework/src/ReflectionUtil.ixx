module;
#include <cstddef>
#include <utility>
#include <limits>
#include <type_traits>
#include <array>
#include <tuple>

export module Util.Reflection;
import Brawler.Functional;

namespace Brawler
{
	namespace Reflection
	{
		namespace IMPL
		{
			template <std::size_t I>
			struct Reflector
			{
				template <typename T>
				constexpr operator T& () const;
			};
		}
	}
}

export namespace Util
{
	namespace Reflection
	{
		template <typename T>
		concept IsReflectable = std::is_default_constructible_v<T> && std::is_aggregate_v<T>;
	}
}

namespace IMPL
{
	template <typename T, std::size_t I0, std::size_t... I>
		requires requires (std::integer_sequence<std::size_t, I0, I...>)
	{
		T{ ::Brawler::Reflection::IMPL::Reflector<I0>{}, ::Brawler::Reflection::IMPL::Reflector<I>{}... };
	}
	consteval void GetFieldCountIMPL(std::size_t& count, std::integer_sequence<std::size_t, I0, I...>)
	{
		count = (sizeof...(I) + 1);
	}

	template <typename T, std::size_t... I>
	consteval void GetFieldCountIMPL(std::size_t& count, std::integer_sequence<std::size_t, I...>)
	{
		GetFieldCountIMPL<T>(count, std::make_integer_sequence<std::size_t, sizeof...(I) - 1>{});
	}
}

export namespace Util
{
	namespace Reflection
	{
		template <typename T>
			requires std::is_aggregate_v<T>
		consteval std::size_t GetFieldCount()
		{
			// If your type has more than 256 fields, then you have other problems which
			// need to be dealt with first.
			constexpr std::size_t CHECK_LIMIT = 256;

			std::size_t count = 0;
			::IMPL::GetFieldCountIMPL<T>(count, std::make_integer_sequence<std::size_t, CHECK_LIMIT>{});

			return count;
		}
	}
}

namespace IMPL
{
	template <typename T, std::size_t NumFields>
	struct TupleCreator
	{
		static_assert(sizeof(T) != sizeof(T), "ERROR: An attempt was made to get the type of a field in a structure, but no implementation for a structure with this number of fields was ever defined! (See IMPL::TupleCreator in ReflectionUtil.ixx.)");
	};

	template <typename T>
	struct TupleCreator<T, 1>
	{
		static consteval auto CreateFieldsTuple()
		{
			const auto [f1] = T{};
			return std::tie(f1);
		}
	};

	template <typename T>
	struct TupleCreator<T, 2>
	{
		static consteval auto CreateFieldsTuple()
		{
			const auto [f1, f2] = T{};
			return std::tie(f1, f2);
		}
	};

	template <typename T>
	struct TupleCreator<T, 3>
	{
		static consteval auto CreateFieldsTuple()
		{
			const auto [f1, f2, f3] = T{};
			return std::tie(f1, f2, f3);
		}
	};

	template <typename T>
	struct TupleCreator<T, 4>
	{
		static consteval auto CreateFieldsTuple()
		{
			const auto [f1, f2, f3, f4] = T{};
			return std::tie(f1, f2, f3, f4);
		}
	};

	template <typename T>
	struct TupleCreator<T, 5>
	{
		static consteval auto CreateFieldsTuple()
		{
			const auto [f1, f2, f3, f4, f5] = T{};
			return std::tie(f1, f2, f3, f4, f5);
		}
	};

	template <typename T>
	struct TupleCreator<T, 6>
	{
		static consteval auto CreateFieldsTuple()
		{
			const auto [f1, f2, f3, f4, f5, f6] = T{};
			return std::tie(f1, f2, f3, f4, f5, f6);
		}
	};

	template <typename T>
	struct TupleCreator<T, 7>
	{
		static consteval auto CreateFieldsTuple()
		{
			const auto [f1, f2, f3, f4, f5, f6, f7] = T{};
			return std::tie(f1, f2, f3, f4, f5, f6, f7);
		}
	};

	template <typename T>
	struct TupleCreator<T, 8>
	{
		static consteval auto CreateFieldsTuple()
		{
			const auto [f1, f2, f3, f4, f5, f6, f7, f8] = T{};
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8);
		}
	};

	template <typename T>
	struct TupleCreator<T, 9>
	{
		static consteval auto CreateFieldsTuple()
		{
			const auto [f1, f2, f3, f4, f5, f6, f7, f8, f9] = T{};
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9);
		}
	};

	template <typename T>
	struct TupleCreator<T, 10>
	{
		static consteval auto CreateFieldsTuple()
		{
			const auto [f1, f2, f3, f4, f5, f6, f7, f8, f9, f10] = T{};
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10);
		}
	};

	template <typename T>
	struct TupleCreator<T, 11>
	{
		static consteval auto CreateFieldsTuple()
		{
			const auto [f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11] = T{};
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11);
		}
	};

	template <typename T>
	struct TupleCreator<T, 12>
	{
		static consteval auto CreateFieldsTuple()
		{
			const auto [f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12] = T{};
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12);
		}
	};

	template <typename T>
	struct TupleCreator<T, 13>
	{
		static consteval auto CreateFieldsTuple()
		{
			const auto [f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13] = T{};
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13);
		}
	};

	template <typename T>
	struct TupleCreator<T, 14>
	{
		static consteval auto CreateFieldsTuple()
		{
			const auto [f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14] = T{};
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14);
		}
	};

	template <typename T>
	struct TupleCreator<T, 15>
	{
		static consteval auto CreateFieldsTuple()
		{
			const auto [f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15] = T{};
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15);
		}
	};

	template <typename T>
	struct TupleCreator<T, 16>
	{
		static consteval auto CreateFieldsTuple()
		{
			const auto [f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16] = T{};
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16);
		}
	};

	template <typename T>
	struct TupleCreator<T, 17>
	{
		static consteval auto CreateFieldsTuple()
		{
			const auto [f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17] = T{};
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17);
		}
	};

	template <typename T>
	struct TupleCreator<T, 18>
	{
		static consteval auto CreateFieldsTuple()
		{
			const auto [f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18] = T{};
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18);
		}
	};

	template <typename T>
	struct TupleCreator<T, 19>
	{
		static consteval auto CreateFieldsTuple()
		{
			const auto [f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19] = T{};
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19);
		}
	};

	template <typename T, std::size_t Index>
	consteval auto GetDefaultFieldValue()
	{
		// It's actually undefined behavior to access the values of the std::tuple returned by
		// TupleCreator::CreateFieldsTuple(), since std::tie() will create a tuple of references
		// to values which are shortly thereafter destroyed. However, we do not access the values
		// of the std::tuple - only their types.

		auto fieldsTuple{ TupleCreator<T, ::Util::Reflection::GetFieldCount<T>()>::CreateFieldsTuple() };
		
		// Rather than use std::get to retrieve a value from the fieldsTuple, we create a new
		// instance of the relevant field type. We do this to avoid undefined behavior. (See the
		// comment above for more details.)
		using ElementType = std::remove_cv_t<std::tuple_element_t<Index, decltype(fieldsTuple)>>;
		return ElementType{};
	}
}

export namespace Util
{
	namespace Reflection
	{
		template <typename T, std::size_t Index>
			requires IsReflectable<T>
		using FieldType = decltype(::IMPL::GetDefaultFieldValue<T, Index>());

		template <typename T, std::size_t Index>
			requires IsReflectable<T>
		consteval std::size_t GetFieldSize()
		{
			return sizeof(FieldType<T, Index>);
		}
	}
}