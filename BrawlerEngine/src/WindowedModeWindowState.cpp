module;
#include <cassert>
#include <DxDef.h>
#include <DirectXMath/DirectXMath.h>

module Brawler.WindowedModeWindowState;
import Brawler.SettingID;
import Brawler.SettingsManager;
import Brawler.AppWindow;
import Brawler.Monitor;
import Util.Engine;

namespace Brawler
{
	WindowedModeWindowState::WindowedModeWindowState(AppWindow& owningWnd) :
		I_WindowState<WindowedModeWindowState>(owningWnd)
	{}

	Win32::WindowMessageResult WindowedModeWindowState::ProcessWindowMessage(const Win32::WindowMessage& msg)
	{
		return Win32::UnhandledMessageResult();
	}

	Win32::CreateWindowInfo WindowedModeWindowState::GetCreateWindowInfo() const
	{
		constexpr std::uint32_t WINDOW_STYLE_WINDOWED_MODE = (WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU | WS_SIZEBOX);
		constexpr std::uint32_t WINDOW_STYLE_EX_WINDOWED_MODE = (WS_EX_OVERLAPPEDWINDOW | WS_EX_LEFT);
		
		DirectX::XMINT2 windowStartCoordinates{
			Brawler::SettingsManager::GetInstance().GetOption<SettingID::WINDOWED_ORIGIN_COORDINATES_X>(),
			Brawler::SettingsManager::GetInstance().GetOption<SettingID::WINDOWED_ORIGIN_COORDINATES_Y>()
		};

		DirectX::XMUINT2 windowSize{
			Brawler::SettingsManager::GetInstance().GetOption<SettingID::WINDOW_RESOLUTION_WIDTH>(),
			Brawler::SettingsManager::GetInstance().GetOption<SettingID::WINDOW_RESOLUTION_HEIGHT>()
		};

		return Win32::CreateWindowInfo{
			.WindowStyle = WINDOW_STYLE_WINDOWED_MODE,
			.WindowStyleEx = WINDOW_STYLE_EX_WINDOWED_MODE,
			.WindowStartCoordinates{ std::move(windowStartCoordinates) },
			.WindowSize{ std::move(windowSize) }
		};
	}

	SwapChainCreationInfo WindowedModeWindowState::GetSwapChainCreationInfo() const
	{
		// Get the client rectangle for the window. This will let us know how large the swap chain's
		// buffers must be. Specifically, we want the swap chain's buffers to be as large as the
		// window. We can apply a scaling pass afterwards if the resolution factor is not 1.0f.
		RECT clientRect{};
		const BOOL getClientRectResult = GetClientRect(GetAppWindow().GetWindowHandle(), &clientRect);

		// I'm not sure why GetClientRect() would fail.
		if (!getClientRectResult) [[unlikely]]
		{
			assert(false && "ERROR: GetClientRect() failed to retrieve the client area RECT of a window!");

			clientRect.right = 0;
			clientRect.bottom = 0;
		}

		Brawler::DXGI_SWAP_CHAIN_DESC windowedModeDesc{ GetDefaultSwapChainDescription() };
		windowedModeDesc.Width = static_cast<std::uint32_t>(clientRect.right);
		windowedModeDesc.Height = static_cast<std::uint32_t>(clientRect.bottom);

		return SwapChainCreationInfo{
			.HWnd = GetAppWindow().GetWindowHandle(),
			.SwapChainDesc{ std::move(windowedModeDesc) },
			.FullscreenDesc{}
		};
	}

	void WindowedModeWindowState::OnShowWindow()
	{}
}