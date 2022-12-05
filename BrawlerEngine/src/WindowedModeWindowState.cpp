module;
#include <cassert>
#include <DxDef.h>
#include <dwmapi.h>
#include <DirectXMath/DirectXMath.h>

module Brawler.WindowedModeWindowState;
import Brawler.SettingID;
import Brawler.SettingsManager;
import Brawler.AppWindow;
import Brawler.Monitor;
import Brawler.MonitorHub;
import Util.Engine;
import Util.General;

namespace
{
	static constexpr std::uint32_t WINDOW_STYLE_WINDOWED_MODE = (WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU | WS_SIZEBOX);
	static constexpr std::uint32_t WINDOW_STYLE_EX_WINDOWED_MODE = (WS_EX_OVERLAPPEDWINDOW | WS_EX_LEFT);
}

namespace Brawler
{
	WindowedModeWindowState::WindowedModeWindowState(AppWindow& owningWnd, const WindowedModeParams& params) :
		I_WindowState<WindowedModeWindowState>(owningWnd),
		mWindowOriginCoordinates(params.WindowOriginCoordinates),
		mWindowSize(params.WindowSize)
	{}

	Win32::WindowMessageResult WindowedModeWindowState::ProcessWindowMessage(const Win32::WindowMessage& msg)
	{
		return Win32::UnhandledMessageResult();
	}

	Win32::CreateWindowInfo WindowedModeWindowState::GetCreateWindowInfo() const
	{
		return Win32::CreateWindowInfo{
			.WindowStyle = WINDOW_STYLE_WINDOWED_MODE,
			.WindowStyleEx = WINDOW_STYLE_EX_WINDOWED_MODE,
			.WindowStartCoordinates{ mWindowOriginCoordinates.GetX(), mWindowOriginCoordinates.GetY() },
			.WindowSize{ mWindowSize.GetX(), mWindowSize.GetY() }
		};
	}

	SwapChainCreationInfo WindowedModeWindowState::GetSwapChainCreationInfo() const
	{
		// Get the Monitor which is most relevant to the associated AppWindow instance.
		const Monitor& bestMonitor{ MonitorHub::GetInstance().GetMonitorFromWindow(GetAppWindow()) };

		// We need to use the client RECT to size the back buffers for the SwapChain; if
		// we use the values in mWindowSize, which specify the size of the entire window,
		// then we will be including the window border in this size. This will cause the
		// generated back buffers to be too large.
		const RECT clientRect{ GetAppWindow().GetClientRect() };

		Brawler::DXGI_SWAP_CHAIN_DESC windowedModeDesc{ GetDefaultSwapChainDescription(bestMonitor) };
		windowedModeDesc.Width = clientRect.right;
		windowedModeDesc.Height = clientRect.bottom;

		return SwapChainCreationInfo{
			.HWnd = GetAppWindow().GetWindowHandle(),
			.SwapChainDesc{ std::move(windowedModeDesc) },
			.FullscreenDesc{}
		};
	}

	void WindowedModeWindowState::UpdateWindowForDisplayMode()
	{
		const Math::Int2 windowRectEndCoords{ mWindowOriginCoordinates + mWindowSize };
		
		UpdateWindowStyle(WindowStyleParams{
			.WindowStyle = WINDOW_STYLE_WINDOWED_MODE,
			.WindowStyleEx = WINDOW_STYLE_EX_WINDOWED_MODE,
			.WindowPosRect{
				.left = mWindowOriginCoordinates.GetX(),
				.top = mWindowOriginCoordinates.GetY(),
				.right = windowRectEndCoords.GetX(),
				.bottom = windowRectEndCoords.GetY()
			}
		});
	}

	void WindowedModeWindowState::UpdateWindowRectangle()
	{
		const RECT windowScreenCoordsRect{ GetAppWindow().GetWindowRect() };

		mWindowOriginCoordinates = Math::Int2{ windowScreenCoordsRect.left, windowScreenCoordsRect.top };
		mWindowSize = Math::UInt2{ static_cast<std::uint32_t>(windowScreenCoordsRect.right - windowScreenCoordsRect.left), static_cast<std::uint32_t>(windowScreenCoordsRect.bottom - windowScreenCoordsRect.top) };
	}
}