module;
#include <DxDef.h>
#include <DirectXMath/DirectXMath.h>

module Brawler.BorderlessWindowedModeWindowState;
import Brawler.SettingID;
import Brawler.SettingsManager;
import Brawler.AppWindow;
import Brawler.Monitor;

namespace Brawler
{
	BorderlessWindowedModeWindowState::BorderlessWindowedModeWindowState(AppWindow& owningWnd) :
		I_WindowState<BorderlessWindowedModeWindowState>(owningWnd)
	{}

	Win32::WindowMessageResult BorderlessWindowedModeWindowState::ProcessWindowMessage(const Win32::WindowMessage& msg)
	{
		return Win32::UnhandledMessageResult();
	}

	Win32::CreateWindowInfo BorderlessWindowedModeWindowState::GetCreateWindowInfo() const
	{
		constexpr std::uint32_t WINDOW_STYLE_BORDERLESS_WINDOWED_MODE = 0;
		constexpr std::uint32_t WINDOW_STYLE_EX_BORDERLESS_WINDOWED_MODE = 0;

		const Brawler::DXGI_OUTPUT_DESC& currMonitorDesc{ GetAppWindow().GetOwningMonitor().GetOutputDescription() };

		DirectX::XMINT2 windowStartCoordinates{
			currMonitorDesc.DesktopCoordinates.left,
			currMonitorDesc.DesktopCoordinates.top
		};

		DirectX::XMUINT2 windowSize{
			static_cast<std::uint32_t>(currMonitorDesc.DesktopCoordinates.right - currMonitorDesc.DesktopCoordinates.left),
			static_cast<std::uint32_t>(currMonitorDesc.DesktopCoordinates.bottom - currMonitorDesc.DesktopCoordinates.top)
		};

		return Win32::CreateWindowInfo{
			.WindowStyle = WINDOW_STYLE_BORDERLESS_WINDOWED_MODE,
			.WindowStyleEx = WINDOW_STYLE_EX_BORDERLESS_WINDOWED_MODE,
			.WindowStartCoordinates{ std::move(windowStartCoordinates) },
			.WindowSize{ std::move(windowSize) }
		};
	}
}