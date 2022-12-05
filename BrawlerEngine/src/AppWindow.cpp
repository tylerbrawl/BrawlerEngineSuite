module;
#include <cassert>
#include <memory>
#include <DxDef.h>
#include <dwmapi.h>

module Brawler.AppWindow;
import Brawler.WindowedModeWindowState;
import Brawler.BorderlessWindowedModeWindowState;
import Brawler.FullscreenModeWindowState;
import Brawler.Manifest;
import Brawler.Application;
import Brawler.SwapChainCreationInfo;
import Util.Threading;
import Brawler.WindowMessageHandler;

namespace Brawler
{
	AppWindow::AppWindow(const WindowedModeParams& windowedModeParams) :
		mSwapChain(),
		mHWnd(),
		mWndStateAdapter(WindowedModeWindowState{ *this, windowedModeParams })
	{
		SpawnWindow();
	}

	AppWindow::AppWindow(const BorderlessWindowedModeParams borderlessWindowedModeParams) :
		mSwapChain(),
		mHWnd(),
		mWndStateAdapter(BorderlessWindowedModeWindowState{ *this, borderlessWindowedModeParams });
	{
		SpawnWindow();
	}

	AppWindow::AppWindow(const FullscreenModeParams& fullscreenModeParams) :
		mSwapChain(),
		mHWnd(),
		mWndStateAdapter(FullscreenModeWindowState{ *this, fullscreenModeParams })
	{
		SpawnWindow();
	}

	Win32::WindowMessageResult AppWindow::ProcessWindowMessage(const Win32::WindowMessage& msg)
	{
		return mWndStateAdapter.AccessData([&msg]<typename WindowState>(WindowState& wndState)
		{
			return wndState.ProcessWindowMessage(msg);
		});
	}

	void AppWindow::ShowWindow(const std::int32_t nCmdShow)
	{
		::ShowWindow(nCmdShow);
	}

	HWND AppWindow::GetWindowHandle() const
	{
		assert(mHWnd.get() != nullptr);
		return mHwnd.get();
	}

	RECT AppWindow::GetWindowRect() const
	{
		// Get the window's bounds in screen coordinates.
		RECT windowScreenCoordsRect{};
		Util::General::CheckHRESULT(DwmGetWindowAttribute(GetWindowHandle(), DWMWINDOWATTRIBUTE::DWMWA_EXTENDED_FRAME_BOUNDS, &windowScreenCoordsRect, sizeof(windowScreenCoordsRect)));

		return windowScreenCoordsRect;
	}

	void AppWindow::EnableWindowedMode(const WindowedModeParams& params)
	{
		mWndStateAdapter = WindowedModeWindowState{ *this, params };
		UpdateWindowForDisplayModeChange();
	}

	void AppWindow::EnableBorderlessWindowedMode(const BorderlessWindowedModeParams params)
	{
		mWndStateAdapter = BorderlessWindowedModeWindowState{ *this, params };
		UpdateWindowForDisplayModeChange();
	}

	void AppWindow::EnableFullscreenMode(const FullscreenModeParams& params)
	{
		mWndStateAdapter = FullscreenModeWindowState{ *this, params };
		UpdateWindowForDisplayModeChange();
	}

	void AppWindow::CreateSwapChain()
	{
		// Create the swap chain for the current mode.
		const SwapChainCreationInfo swapChainInfo{ mWndStateAdapter.AccessData([]<typename WindowState>(const WindowState& wndState)
		{
			return wndState.GetSwapChainCreationInfo();
		}) };

		mSwapChain.CreateSwapChain(swapChainInfo);
	}

	void AppWindow::SpawnWindow()
	{
		// Get the parameters needed to create the window based on its current state.
		const Win32::CreateWindowInfo windowCreationInfo{ mWndStateAdapter.AccessData([]<typename WindowState>(const WindowState& wndState)
		{
			return wndState.GetCreateWindowInfo();
		}) };

		mHWnd.reset(CreateWin32Window(windowCreationInfo));

		if (mHWnd.get() == nullptr) [[unlikely]]
			throw std::runtime_error{ "ERROR: An attempt to create a window for the application failed!" };

		// Register this AppWindow with the WindowMessageHandler.
		WindowMessageHandler::GetInstance().RegisterWindow(*this);

		CreateSwapChain();
	}

	void AppWindow::UpdateWindowForDisplayModeChange()
	{
		assert(mHWnd.get() != nullptr && "ERROR: An attempt was made to call AppWindow::UpdateWindowForDisplayModeChange() before the associated AppWindow could ever create a Win32 window!");
		mWndStateAdapter.AccessData([]<typename WindowState>(WindowState& wndState)
		{
			wndState.UpdateWindowForDisplayMode();
		});

		// Re-build the SwapChain after the display mode changes.
		CreateSwapChain();
	}

	HWND AppWindow::CreateWin32Window(const Win32::CreateWindowInfo& creationInfo)
	{
		assert(Util::Threading::IsMainThread() && "ERROR: Only the main thread can call AppWindow::CreateWin32Window()! (It is assumed that only the main thread will handle Win32 windows and window messages. This is an artifact of the Win32 window message system, since only the thread which creates a window is able to retrieve its window messages.)");

		return CreateWindowEx(
			creationInfo.WindowStyleEx,
			Brawler::Manifest::WINDOW_CLASS_NAME_STR.C_Str(),
			Brawler::Manifest::APPLICATION_NAME.C_Str(),
			creationInfo.WindowStyleEx,
			creationInfo.WindowStartCoordinates.x,
			creationInfo.WindowStartCoordinates.y,
			static_cast<std::uint32_t>(creationInfo.WindowSize.x),
			static_cast<std::uint32_t>(creationInfo.WindowSize.y),
			nullptr,
			nullptr,
			Brawler::GetApplication().GetInstanceHandle(),
			nullptr
		);
	}
}