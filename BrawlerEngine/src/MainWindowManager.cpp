module;
#include <cassert>
#include <memory>
#include <DxDef.h>

module Brawler.MainWindowManager;
import Brawler.MonitorHub;
import Brawler.Monitor;
import Brawler.SettingsManager;
import Brawler.SettingID;
import Brawler.WindowDisplayMode;
import Brawler.BorderlessWindowedModeParams;
import Brawler.FullscreenModeParams;
import Brawler.WindowedModeParams;
import Brawler.Math.MathTypes;

namespace
{
	const Brawler::Monitor& GetMonitorForFullscreenModes()
	{
		// Find the correct monitor by examining the appropriate start coordinates and determining which monitor
		// to start displaying to. If no monitor exists whose desktop coordinates rectangle contains the value
		// we get from the user's configuration, then we default to the primary monitor. This might happen if,
		// for instance, the user disconnects a monitor before launching the application again.

		const Brawler::Math::Int2 fullscreenStartCoordinates{
			Brawler::SettingsManager::GetInstance().GetOption<SettingID::FULLSCREEN_OR_BORDERLESS_ORIGIN_COORDINATES_X>(),
			Brawler::SettingsManager::GetInstance().GetOption<SettingID::FULLSCREEN_OR_BORDERLESS_ORIGIN_COORDINATES_Y>()
		};

		return Brawler::MonitorHub::GetInstance().GetMonitorFromPoint(fullscreenStartCoordinates);
	}
}

namespace Brawler
{
	void MainWindowManager::CreateMainWindow()
	{
		assert(mMainWndPtr == nullptr && "ERROR: An attempt was made to call MainWindowManager::CreateMainWindow() after the main AppWindow instance was already created!");

		// Set the display mode based on the user's configuration file.
		WindowDisplayMode displayMode = SettingsManager::GetInstance().GetOption<SettingID::WINDOW_DISPLAY_MODE>();

		// Verify the value taken from the congfiguration option.
		if (std::to_underlying(displayMode) < 0 || std::to_underlying(displayMode) >= std::to_underlying(WindowDisplayMode::COUNT_OR_ERROR)) [[unlikely]]
		{
			displayMode = WindowDisplayMode::WINDOWED;
			Brawler::SettingsManager::GetInstance().SetOption<SettingID::WINDOW_DISPLAY_MODE>(displayMode);
		}

		switch (displayMode)
		{
		case WindowDisplayMode::MULTI_MONITOR:
		{
			assert(false && "TODO: Implement the multi-monitor display mode!");
			[[fallthrough]];
		}

		case WindowDisplayMode::BORDERLESS_WINDOWED_FULL_SCREEN:
		{
			const BorderlessWindowedModeParams borderlessWindowedModeParams{
				.AssociatedMonitor{ GetMonitorForFullscreenModes() }
			};
			
			mMainWndPtr = std::make_unique<AppWindow>(borderlessWindowedModeParams);
			break;
		}

		case WindowDisplayMode::FULL_SCREEN:
		{
			const FullscreenModeParams fullscreenModeParams{
				.AssociatedMonitor{ GetMonitorForFullscreenModes() },
				.DesiredWidth = SettingsManager::GetInstance().GetOption<SettingID::FULLSCREEN_RESOLUTION_WIDTH>(),
				.DesiredHeight = SettingsManager::GetInstance().GetOption<SettingID::FULLSCREEN_RESOLUTION_HEIGHT>(),
				.DesiredRefreshRate{
					.Numerator = SettingsManager::GetInstance().GetOption<SettingID::FULLSCREEN_REFRESH_RATE_NUMERATOR>(),
					.Denominator = SettingsManager::GetInstance().GetOption<SettingID::FULLSCREEN_REFRESH_RATE_DENOMINATOR>()
				}
			};

			// We don't necessarily need to verify the user input here. If we find that the configuration
			// file has bad values, then FullscreenModeWindowState::GetBestFullscreenMode() will default to
			// the system settings for the specified Monitor.

			mMainWndPtr = std::make_unique<AppWindow>(fullscreenModeParams);
			break;
		}

		case WindowDisplayMode::WINDOWED:
		{
			Math::Int2 windowOriginCoordinates{
				SettingsManager::GetInstance().GetOption<SettingID::WINDOWED_ORIGIN_COORDINATES_X>(),
				SettingsManager::GetInstance().GetOption<SettingID::WINDOWED_ORIGIN_COORDINATES_Y>()
			};

			Math::UInt2 windowSize{
				SettingsManager::GetInstance().GetOption<SettingID::WINDOW_RESOLUTION_WIDTH>(),
				SettingsManager::GetInstance().GetOption<SettingID::WINDOW_RESOLUTION_HEIGHT>()
			};

			// Verify that the values taken from the configuration file are sane. If they aren't, then just
			// use the default window values.
			const RECT userConfigRect{
				.left = windowOriginCoordinates.GetX(),
				.top = windowOriginCoordinates.GetY(),
				.right = (windowOriginCoordinates.GetX() + static_cast<std::uint32_t>(windowSize.GetX())),
				.bottom = (windowOriginCoordinates.GetY() + static_cast<std::uint32_t>(windowSize.GetY()))
			};

			if (userConfigRect.left >= userConfigRect.right || userConfigRect.top >= userConfigRect.bottom) [[unlikely]]
			{
				SettingsManager::GetInstance().ResetOption<SettingID::WINDOWED_ORIGIN_COORDINATES_X>();
				windowOriginCoordinates.SetX(SettingsManager::GetDefaultValueForOption<SettingID::WINDOWED_ORIGIN_COORDINATES_X>());

				SettingsManager::GetInstance().ResetOption<SettingID::WINDOWED_ORIGIN_COORDINATES_Y>();
				windowOriginCoordinates.SetY(SettingsManager::GetDefaultValueForOption<SettingID::WINDOWED_ORIGIN_COORDINATES_Y>());

				SettingsManager::GetInstance().ResetOption<SettingID::WINDOW_RESOLUTION_WIDTH>();
				windowSize.SetX(SettingsManager::GetDefaultValueForOption<SettingID::WINDOW_RESOLUTION_WIDTH>());

				SettingsManager::GetInstance().ResetOption<SettingID::WINDOW_RESOLUTION_HEIGHT>();
				windowSize.SetY(SettingsManager::GetDefaultValueForOption<SettingID::WINDOW_RESOLUTION_HEIGHT>());
			}

			const WindowedModeParams windowedModeParams{
				.WindowOriginCoordinates{ windowOriginCoordinates },
				.WindowSize{ windowSize }
			};

			mMainWndPtr = std::make_unique<AppWindow>(windowedModeParams);
			break;
		}

		default: [[unlikely]]
		{
			assert(false);
			std::unreachable();

			break;
		}
		}
	}

	AppWindow& MainWindowManager::GetMainAppWindow()
	{
		assert(mMainWndPtr != nullptr);
		return *mMainWndPtr;
	}

	const AppWindow& MainWindowManager::GetMainAppWindow() const
	{
		assert(mMainWndPtr != nullptr);
		return *mMainWndPtr;
	}
}