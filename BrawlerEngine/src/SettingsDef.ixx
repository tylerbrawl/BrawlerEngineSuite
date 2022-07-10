module;
#include <type_traits>
#include <string>

export module Brawler.IMPL.SettingsDef;
import Brawler.SettingID;
import Brawler.WindowDisplayMode;
import Brawler.NZStringView;

export namespace Brawler
{
	namespace IMPL
	{
		template <Brawler::SettingID ID>
		struct SettingDefinition
		{
			static_assert(sizeof(ID) != sizeof(ID), "ERROR: An attempt was made to access a SettingDefinition structure for a given SettingID, but no such structure was found!");
		};

		template <Brawler::SettingHeader Header>
		struct SettingHeaderDefinition
		{
			static_assert(sizeof(Header) != sizeof(Header), "ERROR: An attempt was made to access a HeaderDefinition structure for a given SettingHeader, but no such structure was found!");
		};

#define CREATE_HEADER_DEFINITION(header, headerString)																							\
		template <>																																\
		struct SettingHeaderDefinition<header>																									\
		{																																		\
			static constexpr NZStringView HEADER_NAME_STRING{ headerString };																	\
		};

#define CREATE_SETTING_DEFINITION(header, settingID, settingString, defaultValue, settingComment)												\
		template <>																																\
		struct SettingDefinition<settingID>																										\
		{																																		\
			using Type = decltype(defaultValue);																								\
																																				\
			static constexpr Type DEFAULT_VALUE = defaultValue;																					\
			static constexpr Brawler::SettingHeader HEADER = header;																			\
			static constexpr NZStringView OPTION_NAME_STRING{ settingString };																	\
			static constexpr NZStringView OPTION_COMMENT_STRING{ settingComment	};																\
		};

		// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

		// For the most part, we use Hungarian notation for setting names within configuration files. For details on this naming convention, visit
		// https://docs.microsoft.com/en-us/windows/win32/stg/coding-style-conventions.
		//
		// There are, however, some differences:
		//
		//   - f: This prefix denotes floating-point configuration values.
		//   - k: This prefix denotes enumeration (enum class) configuration values.

		CREATE_HEADER_DEFINITION(Brawler::SettingHeader::VIDEO, "Video");
		CREATE_HEADER_DEFINITION(Brawler::SettingHeader::WINDOWS, "Windows");

		CREATE_SETTING_DEFINITION(Brawler::SettingHeader::VIDEO, Brawler::SettingID::WINDOW_RESOLUTION_WIDTH, "uWindowResolutionWidth", 1280u, "# This defines the starting width of the application's window in Windowed Mode.");
		CREATE_SETTING_DEFINITION(Brawler::SettingHeader::VIDEO, Brawler::SettingID::WINDOW_RESOLUTION_HEIGHT, "uWindowResolutionHeight", 720u, "# This defines the starting height of the application's window in Windowed Mode.");
		CREATE_SETTING_DEFINITION(Brawler::SettingHeader::VIDEO, Brawler::SettingID::FULLSCREEN_RESOLUTION_WIDTH, "uFullscreenResolutionWidth", 0u, "# This defines the width of the display mode used in Fullscreen Mode.");
		CREATE_SETTING_DEFINITION(Brawler::SettingHeader::VIDEO, Brawler::SettingID::FULLSCREEN_RESOLUTION_HEIGHT, "uFullscreenResolutionHeight", 0u, "# This defines the height of the display mode used in Fullscreen Mode.");
		CREATE_SETTING_DEFINITION(Brawler::SettingHeader::VIDEO, Brawler::SettingID::FULLSCREEN_REFRESH_RATE_NUMERATOR, "uFullscreenRefreshRateNumerator", 0u, "# This is the numerator of the refresh rate of the display mode used in Fullscreen Mode. The actual refresh rate is calculated as uFullscreenRefreshRateNumerator / uFullscreenRefreshRateDenominator.");
		CREATE_SETTING_DEFINITION(Brawler::SettingHeader::VIDEO, Brawler::SettingID::FULLSCREEN_REFRESH_RATE_DENOMINATOR, "uFullscreenRefreshRateDenominator", 0u, "# This is the denominator of the refresh rate of the display mode used in Fullscreen Mode. The actual refresh rate is calculated as uFullscreenRefreshRateNumerator / uFullscreenRefreshRateDenominator.");

		CREATE_SETTING_DEFINITION(Brawler::SettingHeader::VIDEO, Brawler::SettingID::RENDER_RESOLUTION_FACTOR, "fRenderResolutionFactor", 1.0f, 
			R"(# This is a scalar floating-point value in the range [0.0f, +Infinity[ which is used to scale the resolution at which shading is done. The actual calculated value differs based on the current window display mode:
#
#   - Windowed Mode: The render resolution is calculated as the vector fRenderResolutionFactor * [uWindowResolutionWidth uWindowResolutionHeight].
#   - Borderless Windowed Mode: The render resolution is calculated as the vector fRenderResolutionFactor * [MonitorWidth MonitorHeight].
#   - Fullscreen Mode: The render resolution is calculated as the vector fRenderResolutionFactor * [uFullscreenResolutionWidth uFullscreenResolutionHeight].
#   - Multimonitor Mode: Define vuTotalResolution to be the vector sum of the dimensions of all monitors connected to the relevant display adapter. Then, the render resolution is calculated as fRenderResolutionFactor * vuTotalResolution.)");

		CREATE_SETTING_DEFINITION(Brawler::SettingHeader::VIDEO, Brawler::SettingID::FRAME_RATE_LIMIT, "uFrameRateLimit", 0u, "# This sets a cap to the frames per second (FPS) displayed to the monitor. Setting this value to 0 will result in an uncapped frame rate.");
		CREATE_SETTING_DEFINITION(Brawler::SettingHeader::VIDEO, Brawler::SettingID::WINDOW_DISPLAY_MODE, "kWindowDisplayMode", WindowDisplayMode::WINDOWED,
			R"(# This value determines how the application's window is presented to the user. The following values are available:
#
#   - 0: The application is displayed in Windowed Mode. Only a single window is created, and its resolution can be scaled dynamically. Using this mode may result in increased latency due to DirectFlip not being available.
#   - 1: The application is displayed in Borderless Windowed Mode. Only a single window is created, and the resolution of this window is the same as that of the monitor it is displayed on.
#   - 2: The application is displayed in Fullscreen Mode. Only a single window is created, and its resolution can be configured as needed. Keep in mind that in Direct3D 12, there is no such thing as true exclusive fullscreen mode; 
#     using this will instead enter what is known as emulated fullscreen exclusive (eFSE) mode.
#   - 3: The application is displayed in Multimonitor Mode. Each monitor connected to the display adapter is given a window in the same manner as Borderless Windowed Mode. When only one monitor is being used, this mode is effectively
#     identical to Borderless Windowed Mode. Although eFSE cannot be used in Multimonitor Mode, since each window is taking up the entire dimensions of its associated monitor, Direct3D 12 will be able to make use of DirectFlip, and
#     performance should thus be identical.)");

		CREATE_SETTING_DEFINITION(Brawler::SettingHeader::WINDOWS, Brawler::SettingID::WINDOWED_ORIGIN_COORDINATES_X, "iWindowOriginX", 0, "# This is the X-coordinate of the starting position of the application's window in Windowed Mode.");
		CREATE_SETTING_DEFINITION(Brawler::SettingHeader::WINDOWS, Brawler::SettingID::WINDOWED_ORIGIN_COORDINATES_Y, "iWindowOriginY", 0, "# This is the Y-coordinate of the starting position of the application's window in Windowed Mode.");
		CREATE_SETTING_DEFINITION(Brawler::SettingHeader::WINDOWS, Brawler::SettingID::FULLSCREEN_OR_BORDERLESS_ORIGIN_COORDINATES_X, "iFSBorderlessOriginX", 0, "# This is the X-coordinate of the starting position of the application's window in both Borderless Windowed Mode and Fullscreen Mode.");
		CREATE_SETTING_DEFINITION(Brawler::SettingHeader::WINDOWS, Brawler::SettingID::FULLSCREEN_OR_BORDERLESS_ORIGIN_COORDINATES_Y, "iFSBorderlessOriginY", 0, "# This is the Y-coordinate of the starting position of the application's window in both Borderless Windowed Mode and Fullscreen Mode.");

		// It's not really necessary to #undef a macro at the end of a module interface unit, but it's still good practice.
#undef CREATE_HEADER_DEFINITION
#undef CREATE_SETTING_DEFINITION
	}

	template <Brawler::SettingID ID>
	consteval NZStringView GetSettingIDString()
	{
		return IMPL::SettingDefinition<ID>::OPTION_NAME_STRING;
	}

	template <Brawler::SettingHeader Header>
	consteval NZStringView GetSettingHeaderString()
	{
		return IMPL::SettingHeaderDefinition<Header>::HEADER_NAME_STRING;
	}
}