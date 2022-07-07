module;
#include <DxDef.h>
#include <DirectXMath/DirectXMath.h>

module Brawler.WindowedModeWindowState;
import Brawler.SettingID;
import Brawler.SettingsManager;

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
			.x = Brawler::SettingsManager::GetInstance().GetOption<SettingID::WINDOWED_ORIGIN_COORDINATES_X>(),
			.y = Brawler::SettingsManager::GetInstance().GetOption<SettingID::WINDOWED_ORIGIN_COORDINATES_Y>()
		};

		DirectX::UINT2 windowSize{
			.x = Brawler::SettingsManager::GetInstance().GetOption<SettingID::WINDOW_RESOLUTION_WIDTH>(),
			.y = Brawler::SettingsManager::GetInstance().GetOption<SettingID::WINDOW_RESOLUTION_HEIGHT>()
		};

		return Win32::CreateWindowInfo{
			.WindowStyle = WINDOW_STYLE_WINDOWED_MODE,
			.WindowStyleEx = WINDOW_STYLE_EX_WINDOWED_MODE,
			.WindowStartCoordinates{ std::move(windowStartCoordinates) },
			.WindowSize{ std::move(windowSize) }
		};
	}
}