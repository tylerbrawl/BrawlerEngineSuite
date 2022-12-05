module;
#include <DxDef.h>

module Brawler.I_WindowState;
import Brawler.AppWindow;

namespace Brawler
{
	HWND GetWindowHandle(const AppWindow& window)
	{
		return window.GetWindowHandle();
	}
}