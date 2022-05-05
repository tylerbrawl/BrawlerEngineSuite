module;
#include <string>
#include <type_traits>
#include "DxDef.h"

export module Util.General;

export namespace Util
{
	namespace General
	{
		std::wstring StringToWString(const std::string_view str);
		std::string WStringToString(const std::wstring_view wStr);

		template <typename T>
			requires std::is_enum_v<T>
		constexpr std::underlying_type_t<T> EnumCast(const T enumValue);

		constexpr std::uint64_t CreateStringHash(const std::string_view str);
		constexpr std::uint64_t CreateStringHash(const std::wstring_view str);
	}
}

// -----------------------------------------------------------------------------

namespace Util
{
	namespace General
	{
		template <typename T>
			requires std::is_enum_v<T>
		constexpr std::underlying_type_t<T> EnumCast(const T enumValue)
		{
			return static_cast<std::underlying_type_t<T>>(enumValue);
		}

		constexpr std::uint64_t CreateStringHash(const std::string_view str)
		{
			// Shamelessly copied from http://www.cse.yorku.ca/~oz/hash.html...

			std::uint64_t hash = 5381;

			for (const auto& c : str)
				hash = (((hash << 5) + hash) ^ c);

			return hash;
		}

		constexpr std::uint64_t CreateStringHash(const std::wstring_view str)
		{
			// Shamelessly copied from http://www.cse.yorku.ca/~oz/hash.html...

			std::uint64_t hash = 5381;

			for (const auto& c : str)
				hash = (((hash << 5) + hash) ^ c);

			return hash;
		}
	}
}