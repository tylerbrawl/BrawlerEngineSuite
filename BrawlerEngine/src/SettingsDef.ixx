module;
#include <type_traits>
#include <string>

export module Brawler.IMPL.SettingsDef;
import Brawler.SettingID;

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
			static constexpr const char* ConfigString = headerString;																			\
		};

#define CREATE_SETTING_DEFINITION(header, settingID, settingString, valueType, defaultValue)													\
		template <>																																\
		struct SettingDefinition<settingID>																										\
		{																																		\
			using Type = valueType;																												\
			static_assert(std::is_arithmetic_v<Type>, "ERROR: A SettingDefinition was created with an invalid value type!");					\
																																				\
			static constexpr Type DefaultVal = defaultValue;																					\
			static constexpr Brawler::SettingHeader Header = header;																			\
			static constexpr const char* ConfigString = settingString;																			\
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

		CREATE_SETTING_DEFINITION(Brawler::SettingHeader::VIDEO, Brawler::SettingID::WINDOW_RESOLUTION_WIDTH, "uWindowResolutionWidth", std::uint32_t, 1280);
		CREATE_SETTING_DEFINITION(Brawler::SettingHeader::VIDEO, Brawler::SettingID::WINDOW_RESOLUTION_HEIGHT, "uWindowResolutionHeight", std::uint32_t, 720);
		CREATE_SETTING_DEFINITION(Brawler::SettingHeader::VIDEO, Brawler::SettingID::RENDER_RESOLUTION_FACTOR, "fRenderResolutionFactor", float, 1.0f);
		CREATE_SETTING_DEFINITION(Brawler::SettingHeader::VIDEO, Brawler::SettingID::FRAME_RATE_LIMIT, "uFrameRateLimit", std::uint32_t, 0);

		// Debugging in fullscreen is a nightmare.
#ifdef _DEBUG
		CREATE_SETTING_DEFINITION(Brawler::SettingHeader::VIDEO, Brawler::SettingID::USE_FULLSCREEN, "bUseFullscreen", bool, false);
#else
		CREATE_SETTING_DEFINITION(Brawler::SettingHeader::VIDEO, Brawler::SettingID::USE_FULLSCREEN, "bUseFullscreen", bool, true);
#endif // _DEBUG

		// It's not really necessary to #undef a macro at the end of a module interface unit, but it's still good practice.
#undef CREATE_HEADER_DEFINITION
#undef CREATE_SETTING_DEFINITION
	}

	template <Brawler::SettingID ID>
	constexpr const std::string_view GetSettingIDString()
	{
		return IMPL::SettingDefinition<ID>::ConfigString;
	}

	template <Brawler::SettingHeader Header>
	constexpr const std::string_view GetSettingHeaderString()
	{
		return IMPL::SettingHeaderDefinition<Header>::ConfigString;
	}
}