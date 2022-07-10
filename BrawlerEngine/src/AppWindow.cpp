module;
#include <cassert>
#include <DxDef.h>
#include <DirectXMath/DirectXMath.h>

module Brawler.AppWindow;
import Brawler.WindowedModeWindowState;
import Brawler.BorderlessWindowedModeWindowState;
import Brawler.FullscreenModeWindowState;
import Brawler.Win32.CreateWindowInfo;
import Brawler.Manifest;
import Brawler.Application;
import Brawler.MonitorHub;

namespace Brawler
{
	AppWindow::AppWindow(const Brawler::WindowDisplayMode displayMode) :
		mOwningMonitorPtr(nullptr),
		mSwapChain(),
		mHWnd(nullptr), 
		mWndStateAdapter()
	{
		SetDisplayMode(displayMode);
	}
	
	Win32::WindowMessageResult AppWindow::ProcessWindowMessage(const Win32::WindowMessage& msg)
	{
		return mWndStateAdapter.AccessData([&msg]<typename CurrWindowState>(CurrWindowState& wndState)
		{
			return wndState.ProcessWindowMessage(msg);
		});
	}

	void AppWindow::SpawnWindow()
	{
		// Get the parameters needed to create the window based on its current state.
		Win32::CreateWindowInfo windowCreationInfo{ mWndStateAdapter.AccessData([]<typename WindowState>(const WindowState& wndState)
		{
			return wndState.GetCreateWindowInfo();
		}) };

		mHWnd.reset(Brawler::MonitorHub::GetInstance().GetWindowMessageHandler().RequestWindowCreation(std::move(windowCreationInfo)));

		if (mHWnd.get() == nullptr) [[unlikely]]
			throw std::runtime_error{ "ERROR: An attempt to create a window for the application failed!" };

		// Create the swap chain for the current mode.
		const SwapChainCreationInfo swapChainInfo{ mWndStateAdapter.AccessData([]<typename WindowState>(const WindowState& wndState)
		{
			return wndState.GetSwapChainCreationInfo();
		}) };

		mSwapChain.CreateSwapChain(swapChainInfo);
	}

	void AppWindow::ShowWindow(const bool useInitialCmdShow)
	{
		const std::int32_t nCmdShow = (useInitialCmdShow ? Brawler::GetApplication().GetInitialCmdShow() : SW_SHOW);
		::ShowWindow(mHWnd.get(), nCmdShow);

		if (IsWindowVisible(mHWnd.get())) [[likely]]
		{
			mWndStateAdapter.AccessData([]<typename WindowState>(WindowState& wndState)
			{
				wndState.OnShowWindow();
			});
		}
	}

	HWND AppWindow::GetWindowHandle() const
	{
		assert(mHWnd.get() != nullptr && "ERROR: AppWindow::GetWindowHandle() was called before an AppWindow instance could ever create a window!");
		return mHWnd.get();
	}

	const Monitor& AppWindow::GetOwningMonitor() const
	{
		assert(mOwningMonitorPtr != nullptr && "ERROR: An AppWindow instance was not correctly assigned a pointer to its owning Monitor instance!");
		return *mOwningMonitorPtr;
	}

	void AppWindow::SetOwningMonitor(const Monitor& owningMonitor)
	{
		mOwningMonitorPtr = &owningMonitor;
	}

	void AppWindow::SetDisplayMode(const Brawler::WindowDisplayMode displayMode)
	{
		switch (displayMode)
		{
		case WindowDisplayMode::WINDOWED:
		{
			mWndStateAdapter = WindowedModeWindowState{ *this };
			break;
		}

		case WindowDisplayMode::FULL_SCREEN:
		{
			mWndStateAdapter = FullscreenModeWindowState{ *this };
			break;
		}

		case WindowDisplayMode::MULTI_MONITOR: [[fallthrough]];
		case WindowDisplayMode::BORDERLESS_WINDOWED_FULL_SCREEN:
		{
			mWndStateAdapter = BorderlessWindowedModeWindowState{ *this };
			break;
		}

		default: [[unlikely]]
		{
			assert(false && "ERROR: An unknown Brawler::WindowDisplayMode was provided when constructing an AppWindow instance!");
			std::unreachable();

			mWndStateAdapter = WindowedModeWindowState{ *this };
			break;
		}
		}
	}
}