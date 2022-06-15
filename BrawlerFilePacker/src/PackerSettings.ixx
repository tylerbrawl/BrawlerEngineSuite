module;
#include <cstdint>
#include <array>
#include <cassert>

export module Brawler.PackerSettings;

namespace IMPL
{
	static constexpr std::int32_t ZSTD_DEBUG_COMPRESSION_LEVEL = 1;
	static constexpr std::int32_t ZSTD_RELEASE_COMPRESSION_LEVEL = 19;
}

export namespace Brawler
{
	namespace PackerSettings
	{
		constexpr std::uint32_t TARGET_BCA_VERSION = 1;
		constexpr std::uint32_t TARGET_BPK_VERSION = 1;

		enum class BuildMode : std::uint8_t
		{
			DEBUG,
			RELEASE
		};

		constexpr std::int32_t GetZSTDCompressionLevelForBuildMode(const BuildMode buildMode);

		enum class FilePackerSwitchID : std::uint64_t
		{
			BUILD_FOR_DEBUG			= 1 << 0,
			BUILD_FOR_RELEASE		= 1 << 1
		};

		struct FilePackerSwitch
		{
			const char* CmdLineSwitch;
			const char* Description;
			FilePackerSwitchID SwitchID;
		};

		constexpr FilePackerSwitch BUILD_FOR_DEBUG_SWITCH{
			.CmdLineSwitch = "/D",
			.Description = "Compresses data files using a zstandard compression level suitable for Debug builds. Files will compress *MUCH* faster, but with a smaller compression ratio and slower decompression time.",
			.SwitchID = FilePackerSwitchID::BUILD_FOR_DEBUG
		};

		constexpr FilePackerSwitch BUILD_FOR_RELEASE_SWITCH{
			.CmdLineSwitch = "/R",
			.Description = "Compresses data files using a zstandard compression level suitable for Release builds. Files will compress *MUCH* slower, but with a larger compression ratio and faster decompression time. This is the default setting if neither /D nor /R is specified.",
			.SwitchID = FilePackerSwitchID::BUILD_FOR_RELEASE
		};

		constexpr std::array<FilePackerSwitch, 2> SWITCH_DESCRIPTION_ARR{
			BUILD_FOR_DEBUG_SWITCH,
			BUILD_FOR_RELEASE_SWITCH
		};
	}
}

// ----------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace PackerSettings
	{
		constexpr std::int32_t GetZSTDCompressionLevelForBuildMode(const BuildMode buildMode)
		{
			switch (buildMode)
			{
			case BuildMode::DEBUG:
				return IMPL::ZSTD_DEBUG_COMPRESSION_LEVEL;

			case BuildMode::RELEASE:
				return IMPL::ZSTD_RELEASE_COMPRESSION_LEVEL;

			default:
				assert(false && "ERROR: An attempt was made to get the zstd compression level for an unknown BuildMode!");
				return IMPL::ZSTD_RELEASE_COMPRESSION_LEVEL;
			}
		}
	}
}