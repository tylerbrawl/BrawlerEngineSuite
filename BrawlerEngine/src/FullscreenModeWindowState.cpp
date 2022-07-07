module;
#include <DxDef.h>
#include <DirectXMath/DirectXMath.h>

module Brawler.FullscreenModeWindowState;
import Brawler.AppWindow;
import Brawler.Monitor;
import Brawler.SettingID;
import Brawler.SettingsManager;

namespace Brawler
{
	FullscreenModeWindowState::FullscreenModeWindowState(AppWindow& owningWnd) :
		I_WindowState<FullscreenModeWindowState>(owningWnd)
	{}

	Win32::WindowMessageResult FullscreenModeWindowState::ProcessWindowMessage(const Win32::WindowMessage& msg)
	{
		return Win32::UnhandledMessageResult();
	}

	Win32::CreateWindowInfo FullscreenModeWindowState::GetCreateWindowInfo() const
	{
		constexpr std::uint32_t WINDOW_STYLE_FULLSCREEN_MODE = 0;
		constexpr std::uint32_t WINDOW_STYLE_EX_FULLSCREEN_MODE = 0;

		const Brawler::DXGI_OUTPUT_DESC& currMonitorDesc{ GetAppWindow().GetOwningMonitor().GetOutputDescription() };

		DirectX::XMINT2 windowStartCoordinates{
			.x = currMonitorDesc.DesktopCoordinates.left,
			.y = currMonitorDesc.DesktopCoordinates.top
		};

		DirectX::XMUINT2 windowSize{
			.x = (currMonitorDesc.DesktopCoordinates.right - currMonitorDesc.DesktopCoordinates.left),
			.y = (currMonitorDesc.DesktopCoordinates.bottom - currMonitorDesc.DesktopCoordinates.top)
		};

		return Win32::CreateWindowInfo{
			.WindowStyle = WINDOW_STYLE_FULLSCREEN_MODE,
			.WindowStyleEx = WINDOW_STYLE_EX_FULLSCREEN_MODE,
			.WindowStartCoordinates{ std::move(windowStartCoordinates) },
			.WindowSize{ std::move(windowSize) }
		};
	}
}