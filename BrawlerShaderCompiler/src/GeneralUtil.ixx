module;
#include <string>

export module Util.General;

export namespace Brawler
{
	struct AppParams;
}

export namespace Util
{
	namespace General
	{
		std::wstring StringToWString(const std::string_view str);
		std::string WStringToString(const std::wstring_view wStr);

		const Brawler::AppParams& GetLaunchParameters();

		template <typename T>
			requires std::is_enum_v<T>
		constexpr std::underlying_type_t<T> EnumCast(const T enumValue);
	}
}

// ------------------------------------------------------------------------------

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
	}
}