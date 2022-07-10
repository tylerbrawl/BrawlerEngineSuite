module;
#include <optional>
#include <DxDef.h>
#include <DirectXMath/DirectXMath.h>

module Brawler.BorderlessWindowedModeWindowState;
import Brawler.SettingID;
import Brawler.SettingsManager;
import Brawler.AppWindow;
import Brawler.Monitor;
import Brawler.NZStringView;

namespace Brawler
{
	BorderlessWindowedModeWindowState::BorderlessWindowedModeWindowState(AppWindow& owningWnd) :
		I_WindowState<BorderlessWindowedModeWindowState>(owningWnd)
	{}

	BorderlessWindowedModeWindowState::~BorderlessWindowedModeWindowState()
	{
		// Upon leaving the borderless windowed mode state, we should always show the taskbar
		// if it was hidden.
		ShowTaskbar();
	}

	Win32::WindowMessageResult BorderlessWindowedModeWindowState::ProcessWindowMessage(const Win32::WindowMessage& msg)
	{
		switch (msg.Msg)
		{
		case WM_ACTIVATE:
		{
			const std::uint32_t activationState = LOWORD(msg.WParam);
			const bool isWindowMinimized = HIWORD(msg.WParam);

			const bool isWindowBeingActivated = (activationState == WA_ACTIVE || activationState == WA_CLICKACTIVE);

			// When we have the activated window, AND the window is not currently minimized, hide the
			// taskbar. Otherwise, keep the taskbar visible.
			if (isWindowBeingActivated && !isWindowMinimized)
				HideTaskbar();
			else
				ShowTaskbar();

			return Win32::HandledMessageResult(0);
		}

		default: [[likely]]
			return Win32::UnhandledMessageResult();
		}
	}

	void BorderlessWindowedModeWindowState::OnShowWindow()
	{
		// In borderless windowed mode, hide the taskbar. There doesn't seem to be any documented method
		// for doing this without ugly hacks.
		HideTaskbar();
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
			static_cast<std::uint32_t>(std::abs(currMonitorDesc.DesktopCoordinates.right - currMonitorDesc.DesktopCoordinates.left)),
			static_cast<std::uint32_t>(std::abs(currMonitorDesc.DesktopCoordinates.bottom - currMonitorDesc.DesktopCoordinates.top))
		};

		return Win32::CreateWindowInfo{
			.WindowStyle = WINDOW_STYLE_BORDERLESS_WINDOWED_MODE,
			.WindowStyleEx = WINDOW_STYLE_EX_BORDERLESS_WINDOWED_MODE,
			.WindowStartCoordinates{ std::move(windowStartCoordinates) },
			.WindowSize{ std::move(windowSize) }
		};
	}

	SwapChainCreationInfo BorderlessWindowedModeWindowState::GetSwapChainCreationInfo() const
	{
		const Brawler::DXGI_OUTPUT_DESC& currMonitorDesc{ GetAppWindow().GetOwningMonitor().GetOutputDescription() };

		Brawler::DXGI_SWAP_CHAIN_DESC swapChainDesc{ GetDefaultSwapChainDescription() };
		swapChainDesc.Width = static_cast<std::uint32_t>(std::abs(currMonitorDesc.DesktopCoordinates.right - currMonitorDesc.DesktopCoordinates.left));
		swapChainDesc.Height = static_cast<std::uint32_t>(std::abs(currMonitorDesc.DesktopCoordinates.bottom - currMonitorDesc.DesktopCoordinates.top));

		return SwapChainCreationInfo{
			.HWnd = GetAppWindow().GetWindowHandle(),
			.SwapChainDesc{ std::move(swapChainDesc) },
			.FullscreenDesc{}
		};
	}

	std::optional<HWND> BorderlessWindowedModeWindowState::GetTaskbarWindowForCurrentMonitor() const
	{
		const HMONITOR hMonitor = GetAppWindow().GetOwningMonitor().GetMonitorHandle();

		MONITORINFOEX monitorInfo{};
		monitorInfo.cbSize = sizeof(monitorInfo);

		{
			const BOOL getMonitorInfoResult = GetMonitorInfo(hMonitor, &monitorInfo);

			if (!getMonitorInfoResult) [[unlikely]]
				return std::optional<HWND>{};
		}

		// Try to determine where the taskbar is located by comparing the work area of the monitor to
		// the monitor's display area.
		POINT taskbarPosition{};

		if (monitorInfo.rcMonitor.top != monitorInfo.rcWork.top)
		{
			// The taskbar is anchored to the top of the screen.

			taskbarPosition.x = monitorInfo.rcMonitor.left;
			taskbarPosition.y = monitorInfo.rcMonitor.top;
		}
		else if (monitorInfo.rcMonitor.bottom != monitorInfo.rcWork.bottom)
		{
			// The taskbar is anchored the bottom of the screen.

			taskbarPosition.x = monitorInfo.rcMonitor.left;
			taskbarPosition.y = monitorInfo.rcWork.bottom;
		}
		else if (monitorInfo.rcMonitor.left != monitorInfo.rcWork.left)
		{
			// The taskbar is anchored to the left side of the screen.

			taskbarPosition.x = monitorInfo.rcMonitor.left;
			taskbarPosition.y = monitorInfo.rcMonitor.top;
		}
		else if (monitorInfo.rcMonitor.right != monitorInfo.rcWork.right)
		{
			// The taskbar is anchored to the right side of the screen.

			taskbarPosition.x = monitorInfo.rcWork.right;
			taskbarPosition.y = monitorInfo.rcMonitor.top;
		}
		else [[unlikely]]
		{
			// Um... We couldn't find the taskbar. Did the user hide it or something?

			return std::optional<HWND>{};
		}

		// Get the taskbar's HWND for this monitor.
		const HWND hTaskbarWnd = WindowFromPhysicalPoint(taskbarPosition);
		return (hTaskbarWnd != nullptr ? std::optional<HWND>{ hTaskbarWnd } : std::optional<HWND>{});
	}

	void BorderlessWindowedModeWindowState::ShowTaskbar() const
	{
		const std::optional<HWND> hTaskbarWnd{ GetTaskbarWindowForCurrentMonitor() };

		if (hTaskbarWnd.has_value()) [[likely]]
			ShowWindowAsync(*hTaskbarWnd, SW_SHOW);
	}

	void BorderlessWindowedModeWindowState::HideTaskbar() const
	{
		const std::optional<HWND> hTaskbarWnd{ GetTaskbarWindowForCurrentMonitor() };

		if (hTaskbarWnd.has_value()) [[likely]]
			ShowWindowAsync(*hTaskbarWnd, SW_HIDE);
	}
}