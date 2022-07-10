module;
#include <optional>
#include <DxDef.h>

export module Brawler.Win32.MonitorInformation;

export namespace Brawler
{
	namespace Win32
	{
		struct MonitorInformation
		{
			DISPLAYCONFIG_SOURCE_MODE SourceMode;
			DISPLAYCONFIG_TARGET_MODE TargetMode;
			std::optional<DISPLAYCONFIG_DESKTOP_IMAGE_INFO> DesktopImageInfo;
		};
	}
}