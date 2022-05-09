module;
#include <string>
#include <source_location>
#include "DxDef.h"

export module Util.General;
import Brawler.Functional;

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

		template <typename T>
			requires std::is_enum_v<T>
		constexpr std::underlying_type_t<T> EnumCast(const T enumValue);

#ifdef _DEBUG
		__forceinline void CheckHRESULT(const HRESULT hr, const std::source_location srcLocation = std::source_location::current());
#else
		__forceinline void CheckHRESULT(const HRESULT hr);
#endif // _DEBUG

		enum class BuildMode
		{
			DEBUG,
			RELEASE_WITH_DEBUGGING,
			RELEASE
		};

		consteval BuildMode GetBuildMode();

		/// <summary>
		/// Determines whether the current build is a Debug build or not. Note that
		/// Release with Debugging builds do *NOT* count as Debug builds, so this function
		/// will return false in those builds.
		/// </summary>
		/// <returns>
		/// The function returns true if the program is being compiled into a Debug build 
		/// and false otherwise.
		/// </returns>
		consteval bool IsDebugModeEnabled();

		/// <summary>
		/// In both Debug builds and in Release with Debugging builds, this function will
		/// trigger a breakpoint programmatically. It is preferred over calling the raw Win32
		/// DebugBreak() function.
		/// 
		/// Using this function can allow you to place breakpoints in code locations which
		/// Visual Studio would otherwise fail to place one for you (e.g., within template
		/// function definitions).
		/// 
		/// In Release builds, this function does nothing.
		/// </summary>
		__forceinline void DebugBreak();
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

#ifdef _DEBUG
		__forceinline void CheckHRESULT(const HRESULT hr, const std::source_location srcLocation)
		{
			if (FAILED(hr)) [[unlikely]]
			{
				const _com_error comErr{ hr };
				throw std::runtime_error{ std::string{"An HRESULT check failed!\n\nHRESULT Returned: "} + WStringToString(comErr.ErrorMessage()) +
					"\nFunction: " + srcLocation.function_name() + "\nFile : " + srcLocation.file_name() + " (Line Number : " + std::to_string(srcLocation.line()) + ")" };
			}
		}
#else
		__forceinline void CheckHRESULT(const HRESULT hr)
		{
			if (FAILED(hr)) [[unlikely]]
				throw std::runtime_error{ "" };
		}
#endif // _DEBUG

		consteval BuildMode GetBuildMode()
		{
#ifdef _DEBUG
			return BuildMode::DEBUG;

#elif defined(__RELEASE_WITH_DEBUGGING__)  // Seriously, Microsoft, how hard can it be to add #elifdef?
			return BuildMode::RELEASE_WITH_DEBUGGING;

#else
			return BuildMode::RELEASE;
#endif // _DEBUG
		}

		consteval bool IsDebugModeEnabled()
		{
			return (GetBuildMode() == BuildMode::DEBUG);
		}

		__forceinline void DebugBreak()
		{
			if constexpr (GetBuildMode() != BuildMode::RELEASE)
				::DebugBreak();
		}
	}
}