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
		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(U& val)
		{
			auto& [f1] = val;
			return std::tie(f1);
		}

		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(const U& val)
		{
			const auto& [f1] = val;
			return std::tie(f1);
		}
	};

	template <typename T>
	struct TupleCreator<T, 2>
	{
		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(U& val)
		{
			auto& [f1, f2] = val;
			return std::tie(f1, f2);
		}

		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(const U& val)
		{
			const auto& [f1, f2] = val;
			return std::tie(f1, f2);
		}
	};

	template <typename T>
	struct TupleCreator<T, 3>
	{
		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(U& val)
		{
			auto& [f1, f2, f3] = val;
			return std::tie(f1, f2, f3);
		}

		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(const U& val)
		{
			const auto& [f1, f2, f3] = val;
			return std::tie(f1, f2, f3);
		}
	};

	template <typename T>
	struct TupleCreator<T, 4>
	{
		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(U& val)
		{
			auto& [f1, f2, f3, f4] = val;
			return std::tie(f1, f2, f3, f4);
		}

		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(const U& val)
		{
			const auto& [f1, f2, f3, f4] = val;
			return std::tie(f1, f2, f3, f4);
		}
	};

	template <typename T>
	struct TupleCreator<T, 5>
	{
		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(U& val)
		{
			auto& [f1, f2, f3, f4, f5] = val;
			return std::tie(f1, f2, f3, f4, f5);
		}

		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(const U& val)
		{
			const auto& [f1, f2, f3, f4, f5] = val;
			return std::tie(f1, f2, f3, f4, f5);
		}
	};

	template <typename T>
	struct TupleCreator<T, 6>
	{
		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(U& val)
		{
			auto& [f1, f2, f3, f4, f5, f6] = val;
			return std::tie(f1, f2, f3, f4, f5, f6);
		}

		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(const U& val)
		{
			const auto& [f1, f2, f3, f4, f5, f6] = val;
			return std::tie(f1, f2, f3, f4, f5, f6);
		}
	};

	template <typename T>
	struct TupleCreator<T, 7>
	{
		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(U& val)
		{
			auto& [f1, f2, f3, f4, f5, f6, f7] = val;
			return std::tie(f1, f2, f3, f4, f5, f6, f7);
		}

		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(const U& val)
		{
			const auto& [f1, f2, f3, f4, f5, f6, f7] = val;
			return std::tie(f1, f2, f3, f4, f5, f6, f7);
		}
	};

	template <typename T>
	struct TupleCreator<T, 8>
	{
		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(U& val)
		{
			auto& [f1, f2, f3, f4, f5, f6, f7, f8] = val;
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8);
		}

		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(const U& val)
		{
			const auto& [f1, f2, f3, f4, f5, f6, f7, f8] = val;
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8);
		}
	};

	template <typename T>
	struct TupleCreator<T, 9>
	{
		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(U& val)
		{
			auto& [f1, f2, f3, f4, f5, f6, f7, f8, f9] = val;
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9);
		}

		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(const U& val)
		{
			const auto& [f1, f2, f3, f4, f5, f6, f7, f8, f9] = val;
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9);
		}
	};

	template <typename T>
	struct TupleCreator<T, 10>
	{
		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(U& val)
		{
			auto& [f1, f2, f3, f4, f5, f6, f7, f8, f9, f10] = val;
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10);
		}

		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(const U& val)
		{
			const auto& [f1, f2, f3, f4, f5, f6, f7, f8, f9, f10] = val;
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10);
		}
	};

	template <typename T>
	struct TupleCreator<T, 11>
	{
		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(U& val)
		{
			auto& [f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11] = val;
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11);
		}

		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(const U& val)
		{
			const auto& [f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11] = val;
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11);
		}
	};

	template <typename T>
	struct TupleCreator<T, 12>
	{
		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(U& val)
		{
			auto& [f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12] = val;
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12);
		}

		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(const U& val)
		{
			const auto& [f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12] = val;
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12);
		}
	};

	template <typename T>
	struct TupleCreator<T, 13>
	{
		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(U& val)
		{
			auto& [f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13] = val;
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13);
		}

		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(const U& val)
		{
			const auto& [f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13] = val;
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13);
		}
	};

	template <typename T>
	struct TupleCreator<T, 14>
	{
		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(U& val)
		{
			auto& [f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14] = val;
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14);
		}

		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(const U& val)
		{
			const auto& [f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14] = val;
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14);
		}
	};

	template <typename T>
	struct TupleCreator<T, 15>
	{
		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(U& val)
		{
			auto& [f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15] = val;
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15);
		}

		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(const U& val)
		{
			const auto& [f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15] = val;
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15);
		}
	};

	template <typename T>
	struct TupleCreator<T, 16>
	{
		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(U& val)
		{
			auto& [f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16] = val;
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16);
		}

		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(const U& val)
		{
			const auto& [f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16] = val;
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16);
		}
	};

	template <typename T>
	struct TupleCreator<T, 17>
	{
		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(U& val)
		{
			auto& [f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17] = val;
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17);
		}

		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(const U& val)
		{
			const auto& [f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17] = val;
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17);
		}
	};

	template <typename T>
	struct TupleCreator<T, 18>
	{
		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(U& val)
		{
			auto& [f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18] = val;
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18);
		}

		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(const U& val)
		{
			const auto& [f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18] = val;
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18);
		}
	};

	template <typename T>
	struct TupleCreator<T, 19>
	{
		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(U& val)
		{
			auto& [f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19] = val;
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19);
		}

		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(const U& val)
		{
			const auto& [f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19] = val;
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19);
		}
	};

	template <typename T>
	struct TupleCreator<T, 20>
	{
		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(U& val)
		{
			auto& [f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20] = val;
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20);
		}

		template <typename U>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
		static constexpr auto CreateFieldsTuple(const U& val)
		{
			const auto& [f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20] = val;
			return std::tie(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20);
		}
	};

	template <typename T, std::size_t Index>
	consteval auto GetDefaultFieldValue()
	{
		T dataInstance{};
		auto fieldsTuple{ TupleCreator<T, ::Util::Reflection::GetFieldCount<T>()>::CreateFieldsTuple(dataInstance) };

		return std::get<Index>(fieldsTuple);
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

		template <typename T, std::size_t Index>
			requires IsReflectable<T>
		constexpr auto& GetFieldReference(T& data)
		{
			auto dataFieldsTuple{ IMPL::TupleCreator<T, Util::Reflection::GetFieldCount<T>()>::CreateFieldsTuple(data) };
			return std::get<Index>(dataFieldsTuple);
		}

		template <typename T, std::size_t Index>
			requires IsReflectable<T>
		constexpr const auto& GetFieldReference(const T& data)
		{
			const auto dataFieldsTuple{ IMPL::TupleCreator<T, Util::Reflection::GetFieldCount<T>()>::CreateFieldsTuple(data) };
			return std::get<Index>(dataFieldsTuple);
		}
	}
}