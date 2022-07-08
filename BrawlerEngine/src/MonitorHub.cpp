#define OEMRESOURCE

module;
#include <vector>
#include <memory>
#include <cassert>
#include <DxDef.h>

module Brawler.MonitorHub;
import Util.Engine;
import Util.General;
import Brawler.Win32.WindowMessage;
import Brawler.NZStringView;
import Brawler.Application;
import Brawler.Manifest;
import Brawler.SettingID;
import Brawler.SettingsManager;
import Brawler.WindowDisplayMode;

namespace
{
	static LRESULT WndProc(
		const HWND hWnd,
		const UINT uMsg,
		const WPARAM wParam,
		const LPARAM lParam
	)
	{
		const Brawler::Win32::WindowMessage windowMsg{
			.Msg = uMsg,
			.WParam = wParam,
			.LParam = lParam
		};
		
		const Brawler::Win32::WindowMessageResult msgResult{ Brawler::MonitorHub::GetInstance().ProcessWindowMessage(hWnd, windowMsg) };
		return (msgResult.MessageHandled ? msgResult.Result : DefWindowProc(hWnd, uMsg, wParam, lParam));
	}

	void RegisterBrawlerEngineWindowClass()
	{
		constexpr WNDCLASSEX DEFAULT_WINDOW_CLASS_VALUE{
			.cbSize = sizeof(WNDCLASSEX),
			.style = CS_DROPSHADOW,
			.lpfnWndProc = WndProc,
			.lpszClassName = Brawler::Manifest::WINDOW_CLASS_NAME_STR.C_Str()
		};

		WNDCLASSEX wndClass{ DEFAULT_WINDOW_CLASS_VALUE };
		wndClass.hInstance = Brawler::GetApplication().GetInstanceHandle();

		// TODO: Allow for the application to use a different HICON to change how it appears in
		// the taskbar, among other places.

		wndClass.hCursor = reinterpret_cast<HCURSOR>(LoadImage(
			nullptr,
			MAKEINTRESOURCE(OCR_NORMAL),
			IMAGE_CURSOR,
			0,
			0,
			LR_DEFAULTSIZE | LR_SHARED
		));

		const ATOM registerClassResult = RegisterClassEx(&wndClass);

		if (registerClassResult == 0) [[unlikely]]
			throw std::runtime_error{ "ERROR: The application's window class could not be registered!" };
	}
}

namespace Brawler
{
	MonitorHub::MonitorHub() :
		mMonitorArr(),
		mHWndMap()
	{
		RegisterBrawlerEngineWindowClass();
		EnumerateDisplayOutputs();

		ResetApplicationWindows();
	}

	MonitorHub& MonitorHub::GetInstance()
	{
		static MonitorHub instance{};
		return instance;
	}

	Win32::WindowMessageResult MonitorHub::ProcessWindowMessage(const HWND hWnd, const Win32::WindowMessage& msg)
	{
		assert(mHWndMap.contains(hWnd) && "ERROR: MonitorHub::ProcessWindowMessage() was called with an HWND which the MonitorHub instance never registered!");
		assert(mHWndMap.at(hWnd) != nullptr);

		return mHWndMap.at(hWnd)->ProcessWindowMessage(msg);
	}

	void MonitorHub::EnumerateDisplayOutputs()
	{
		Brawler::DXGIAdapter& dxgiAdapter{ Util::Engine::GetDXGIAdapter() };

		// Get the first adapter. If this fails, then we throw an exception, because there
		// should be at least one connected adapter.
		{
			Microsoft::WRL::ComPtr<IDXGIOutput> oldDXGIOutputPtr{};
			const HRESULT hr = dxgiAdapter.EnumOutputs(0, &oldDXGIOutputPtr);

			if (FAILED(hr)) [[unlikely]]
				throw std::runtime_error{ "ERROR: A display output could not be found!" };

			Microsoft::WRL::ComPtr<Brawler::DXGIOutput> newDXGIOutputPtr{};
			Util::General::CheckHRESULT(oldDXGIOutputPtr.As(&newDXGIOutputPtr));

			mMonitorArr.push_back(std::make_unique<Monitor>(std::move(newDXGIOutputPtr)));
		}

		// Continue checking for additional monitors.
		std::uint32_t currIndex = 1;
		bool doMoreOutputsExist = true;

		while (doMoreOutputsExist)
		{
			Microsoft::WRL::ComPtr<IDXGIOutput> oldDXGIOutputPtr{};
			const HRESULT hr = dxgiAdapter.EnumOutputs(currIndex++, &oldDXGIOutputPtr);

			switch (hr)
			{
			// S_OK: The operation completed successfully (i.e., we found another monitor).
			case S_OK:
			{
				Microsoft::WRL::ComPtr<Brawler::DXGIOutput> newDXGIOutputPtr{};
				Util::General::CheckHRESULT(oldDXGIOutputPtr.As(&newDXGIOutputPtr));

				mMonitorArr.push_back(std::make_unique<Monitor>(std::move(newDXGIOutputPtr)));

				break;
			}

			// DXGI_ERROR_NOT_FOUND: currIndex has exceeded the number of connected outputs. In this
			// case, we have found all of the monitors, and can thus exit.
			case DXGI_ERROR_NOT_FOUND:
			{
				doMoreOutputsExist = false;
				break;
			}

			default: [[unlikely]]
			{
				Util::General::CheckHRESULT(hr);

				doMoreOutputsExist = false;
				break;
			}
			}
		}
	}

	void MonitorHub::ResetApplicationWindows()
	{
		// Close all active windows.
		for (auto& monitorPtr : mMonitorArr)
			monitorPtr->ResetWindow();

		// Delete the HWND map.
		mHWndMap.clear();

		const WindowDisplayMode displayMode{ Brawler::SettingsManager::GetInstance().GetOption<SettingID::WINDOW_DISPLAY_MODE>() };

		switch (displayMode)
		{
		case WindowDisplayMode::MULTI_MONITOR:
		{
			// If the current WindowDisplayMode is for multi-monitor display, then we create a new window for every
			// monitor. Otherwise, we create a single window for a single monitor.
			for (auto& monitorPtr : mMonitorArr)
				CreateWindowForMonitor(*monitorPtr, displayMode);

			break;
		}

		case WindowDisplayMode::BORDERLESS_WINDOWED_FULL_SCREEN: [[fallthrough]];
		case WindowDisplayMode::FULL_SCREEN:
		{
			// Find the correct monitor by examining the appropriate start coordinates and determining which monitor
			// to start displaying to. If no monitor exists whose desktop coordinates rectangle contains the value
			// we get from the user's configuration, then we default to the primary monitor. This might happen if,
			// for instance, the user disconnects a monitor before launching the application again.
			const DirectX::XMINT2 fullscreenStartCoordinates{
				Brawler::SettingsManager::GetInstance().GetOption<SettingID::FULLSCREEN_OR_BORDERLESS_ORIGIN_COORDINATES_X>(),
				Brawler::SettingsManager::GetInstance().GetOption<SettingID::FULLSCREEN_OR_BORDERLESS_ORIGIN_COORDINATES_Y>()
			};

			bool windowCreated = false;
			for (auto& monitorPtr : mMonitorArr)
			{
				const RECT& monitorDesktopCoords{ monitorPtr->GetOutputDescription().DesktopCoordinates };

				if (monitorDesktopCoords.left <= fullscreenStartCoordinates.x && monitorDesktopCoords.right > fullscreenStartCoordinates.x &&
					monitorDesktopCoords.top <= fullscreenStartCoordinates.y && monitorDesktopCoords.bottom > fullscreenStartCoordinates.y)
				{
					CreateWindowForMonitor(*monitorPtr, displayMode);
					windowCreated = true;

					break;
				}
			}

			if (!windowCreated) [[unlikely]]
				CreateWindowForMonitor(GetPrimaryMonitor(), displayMode);

			break;
		}

		case WindowDisplayMode::WINDOWED:
		{
			// Get the window rectangle from the user's configuration and compare it to the desktop coordinates of
			// every connected monitor. The monitor whose desktop coordinates RECT has the greatest area of intersection
			// with the configuration RECT gets the new window.
			const DirectX::XMINT2 windowedStartCoordinates{
				Brawler::SettingsManager::GetInstance().GetOption<SettingID::WINDOWED_ORIGIN_COORDINATES_X>(),
				Brawler::SettingsManager::GetInstance().GetOption<SettingID::WINDOWED_ORIGIN_COORDINATES_Y>()
			};

			const DirectX::XMUINT2 windowSize{
				Brawler::SettingsManager::GetInstance().GetOption<SettingID::WINDOW_RESOLUTION_WIDTH>(),
				Brawler::SettingsManager::GetInstance().GetOption<SettingID::WINDOW_RESOLUTION_HEIGHT>()
			};

			const RECT userConfigRect{
				.left = windowedStartCoordinates.x,
				.top = windowedStartCoordinates.y,
				.right = (windowedStartCoordinates.x + static_cast<std::int32_t>(windowSize.x)),
				.bottom = (windowedStartCoordinates.y + static_cast<std::int32_t>(windowSize.y))
			};

			// Verify that the values taken from the configuration file are sane. If they aren't, then just use
			// the default window values.
			if (userConfigRect.left > userConfigRect.right || userConfigRect.top > userConfigRect.bottom) [[unlikely]]
			{
				Brawler::SettingsManager::GetInstance().ResetOption<SettingID::WINDOWED_ORIGIN_COORDINATES_X>();
				Brawler::SettingsManager::GetInstance().ResetOption<SettingID::WINDOWED_ORIGIN_COORDINATES_Y>();
				Brawler::SettingsManager::GetInstance().ResetOption<SettingID::WINDOW_RESOLUTION_WIDTH>();
				Brawler::SettingsManager::GetInstance().ResetOption<SettingID::WINDOW_RESOLUTION_HEIGHT>();

				CreateWindowForMonitor(GetPrimaryMonitor(), displayMode);

				break;
			}

			Monitor* monitorWithGreatestIntersectionAreaPtr = nullptr;
			float largestIntersectionAreaFound = std::numeric_limits<float>::min();

			for (auto& monitorPtr : mMonitorArr)
			{
				const RECT& monitorDesktopCoords{ monitorPtr->GetOutputDescription().DesktopCoordinates };
				const RECT intersectionRect{
					.left = std::max(userConfigRect.left, monitorDesktopCoords.left),
					.top = std::max(userConfigRect.top, monitorDesktopCoords.top),
					.right = std::min(userConfigRect.right, monitorDesktopCoords.right),
					.bottom = std::min(userConfigRect.bottom, monitorDesktopCoords.bottom)
				};

				// If intersectionRect.left > intersectionRect.right and/or intersectionRect.top > intersectionRect.bottom,
				// then the rectangles do *NOT* intersect.
				if (intersectionRect.left <= intersectionRect.right && intersectionRect.top <= intersectionRect.bottom)
				{
					const float intersectionArea = static_cast<float>(intersectionRect.right - intersectionRect.left) * static_cast<float>(intersectionRect.bottom - intersectionRect.top);

					if (intersectionArea > largestIntersectionAreaFound) [[likely]]
					{
						monitorWithGreatestIntersectionAreaPtr = monitorPtr.get();
						largestIntersectionAreaFound = intersectionArea;
					}
				}
			}

			if (monitorWithGreatestIntersectionAreaPtr != nullptr) [[likely]]
				CreateWindowForMonitor(*monitorWithGreatestIntersectionAreaPtr, displayMode);
			else [[unlikely]]
				CreateWindowForMonitor(GetPrimaryMonitor(), displayMode);

			break;
		}

		default: [[unlikely]]
		{
			assert(false && "ERROR: An unrecognized Brawler::WindowDisplayMode value was detected when trying to reset the application's windows!");
			std::unreachable();

			break;
		}
		}
	}

	Monitor& MonitorHub::GetPrimaryMonitor()
	{
		assert(!mMonitorArr.empty());
		assert(mMonitorArr[0] != nullptr);

		return *(mMonitorArr[0]);
	}

	const Monitor& MonitorHub::GetPrimaryMonitor() const
	{
		assert(!mMonitorArr.empty());
		assert(mMonitorArr[0] != nullptr);

		return *(mMonitorArr[0]);
	}

	void MonitorHub::CreateWindowForMonitor(Monitor& monitor, const WindowDisplayMode displayMode)
	{
		std::unique_ptr<AppWindow> appWindowPtr{ std::make_unique<AppWindow>(displayMode) };
		AppWindow* const rawAppWindowPtr = appWindowPtr.get();

		monitor.AssignWindow(std::move(appWindowPtr));
		
		// Have the Monitor instance call AppWindow::SpawnWindow() on the AppWindow instance which we just
		// assigned it. This will assign the AppWindow an HWND value, allowing it to receive window messages.
		monitor.SpawnWindowForMonitor();

		// Associate the HWND with its corresponding AppWindow instance. We need to do this in order to
		// process window messages.
		mHWndMap.try_emplace(rawAppWindowPtr->GetWindowHandle(), rawAppWindowPtr);
	}
}