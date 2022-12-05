module;
#include <memory>
#include <DxDef.h>

export module Brawler.AppWindow;
import Brawler.SwapChain;
import Brawler.Win32.WindowMessage;
import Brawler.PolymorphicAdapter;
import Brawler.I_WindowState;
export import Brawler.WindowStateTraits;
import Brawler.WindowedModeParams;
import Brawler.BorderlessWindowedModeParams;
import Brawler.FullscreenModeParams;
import Brawler.Win32.CreateWindowInfo;
import Brawler.Math.MathTypes;

namespace Brawler
{
	struct WindowDeleter
	{
		void operator()(const HWND hWnd) const
		{
			if (hWnd != nullptr) [[likely]]
				DestroyWindow(hWnd);
		}
	};

	using SafeWindow = std::unique_ptr<std::remove_pointer_t<HWND>, WindowDeleter>;
}

export namespace Brawler
{
	class AppWindow
	{
	public:
		explicit AppWindow(const WindowedModeParams& windowedModeParams);
		explicit AppWindow(const BorderlessWindowedModeParams borderlessWindowedModeParams);
		explicit AppWindow(const FullscreenModeParams& fullscreenModeParams);

		AppWindow(const AppWindow& rhs) = delete;
		AppWindow& operator=(const AppWindow& rhs) = delete;

		AppWindow(AppWindow&& rhs) noexcept = default;
		AppWindow& operator=(AppWindow&& rhs) noexcept = default;

		Win32::WindowMessageResult ProcessWindowMessage(const Win32::WindowMessage& msg);

		void ShowWindow(const std::int32_t nCmdShow);

		HWND GetWindowHandle() const;

		/// <summary>
		/// Obtains the RECT specifying the client area coordinates of the AppWindow instance.
		/// Following Win32 conventions, the top and left fields of the returned structure
		/// are contained within the rectangle represented by RECT, while the bottom and
		/// right fields lie just outside of the rectangle.
		/// 
		/// The client area of a Win32 window includes every part of the window which can
		/// typically be drawn to using Win32 APIs. It does not include things like the top
		/// border of a traditional window. The upper-left coordinates of every RECT returned
		/// by AppWindow::GetClientRect()
		/// 
		/// Importantly, the size of the RECT returned by AppWindow::GetClientRect() matches the 
		/// dimensions of the back buffers owned by the SwapChain associated with the relevant 
		/// AppWindow instance. This is not the case for AppWindow::GetWindowRect(), which includes 
		/// the window borders.
		/// </summary>
		RECT GetClientRect() const;

		/// <summary>
		/// Obtains the RECT specifying the screen coordinates of the AppWindow instance.
		/// Following Win32 conventions, the top and left fields of the returned structure
		/// are contained within the rectangle represented by RECT, while the bottom and
		/// right fields lie just outside of the rectangle.
		/// 
		/// The RECT returned does *NOT* include the window's drop shadow, unlike the returned
		/// RECT in the Win32 ::GetWindowRect() function, which does include it (unless the
		/// window has never been shown prior to the call).
		/// </summary>
		RECT GetWindowRect() const;

		/// <summary>
		/// Calculates and returns the suggested resolution of render targets whose outputs
		/// are to be copied to the back buffers of the SwapChain owned by this AppWindow
		/// instance. The returned value is appropriately scaled based on the render resolution
		/// factor set in the user's configuration file.
		/// </summary>
		Math::UInt2 GetSuggestedRenderResolution() const;

		void EnableWindowedMode(const WindowedModeParams& params);
		void EnableBorderlessWindowedMode(const BorderlessWindowedModeParams& params);
		void EnableFullscreenMode(const FullscreenModeParams& params);

		void CreateSwapChain();

	private:
		void SpawnWindow();
		void UpdateWindowForDisplayModeChange();

		static HWND CreateWin32Window(const Win32::CreateWindowInfo& creationInfo);

	private:
		SwapChain mSwapChain;
		SafeWindow mHWnd;

		// The PolymorphicAdapter for the window state should always be created last and
		// destroyed first.
		PolymorphicAdapter<I_WindowState> mWndStateAdapter;
	};
}