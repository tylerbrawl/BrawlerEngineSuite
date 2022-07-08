module;
#include <DxDef.h>

export module Brawler.WindowedModeWindowState;
import Brawler.I_WindowState;
import Brawler.Win32.WindowMessage;
import Brawler.Win32.CreateWindowInfo;

export namespace Brawler
{
	class WindowedModeWindowState final : public I_WindowState<WindowedModeWindowState>
	{
	public:
		explicit WindowedModeWindowState(AppWindow& owningWnd);

		WindowedModeWindowState(const WindowedModeWindowState& rhs) = delete;
		WindowedModeWindowState& operator=(const WindowedModeWindowState& rhs) = delete;

		WindowedModeWindowState(WindowedModeWindowState&& rhs) noexcept = default;
		WindowedModeWindowState& operator=(WindowedModeWindowState&& rhs) noexcept = default;

		Win32::WindowMessageResult ProcessWindowMessage(const Win32::WindowMessage& msg);
		Win32::CreateWindowInfo GetCreateWindowInfo() const;

		SwapChainCreationInfo GetSwapChainCreationInfo() const;
	};
}