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
		RECT GetWindowRect() const;

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