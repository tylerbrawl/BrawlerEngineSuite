module;
#include <cassert>
#include <memory>
#include <DxDef.h>
#include <DirectXMath/DirectXMath.h>

export module Brawler.AppWindow;
import Brawler.Win32.WindowMessage;
import Brawler.WindowDisplayMode;
import Brawler.PolymorphicAdapter;
import Brawler.I_WindowState;
export import Brawler.WindowStateTraits;
import Brawler.SwapChain;

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
	class Monitor;
}

export namespace Brawler
{
	class AppWindow
	{
	public:
		explicit AppWindow(const Brawler::WindowDisplayMode displayMode);

		AppWindow(const AppWindow& rhs) = delete;
		AppWindow& operator=(const AppWindow& rhs) = delete;

		AppWindow(AppWindow&& rhs) noexcept = default;
		AppWindow& operator=(AppWindow&& rhs) noexcept = default;

		Win32::WindowMessageResult ProcessWindowMessage(const Win32::WindowMessage& msg);

		void SpawnWindow();
		void ShowWindow(const bool useInitialCmdShow);

		HWND GetWindowHandle() const;

		const Monitor& GetOwningMonitor() const;
		void SetOwningMonitor(const Monitor& owningMonitor);

		void SetDisplayMode(const Brawler::WindowDisplayMode displayMode);

	private:
		const Monitor* mOwningMonitorPtr;
		SwapChain mSwapChain;
		SafeWindow mHWnd;

		// The PolymorphicAdapter for the window state should always be created last and
		// destroyed first.
		PolymorphicAdapter<I_WindowState> mWndStateAdapter;
	};
}