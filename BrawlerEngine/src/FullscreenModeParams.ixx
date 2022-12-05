module;
#include <DxDef.h>

export module Brawler.FullscreenModeParams;
import Brawler.Monitor;

export namespace Brawler
{
	struct FullscreenModeParams
	{
		const Monitor& AssociatedMonitor;
		
		std::uint32_t DesiredWidth;
		std::uint32_t DesiredHeight;
		DXGI_RATIONAL DesiredRefreshRate;
	};
}