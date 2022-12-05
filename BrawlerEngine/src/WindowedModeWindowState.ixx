module;
#include <DxDef.h>

export module Brawler.WindowedModeWindowState;
import Brawler.I_WindowState;
import Brawler.WindowedModeParams;
import Brawler.Math.MathTypes;
import Brawler.Win32.WindowMessage;
import Brawler.Win32.CreateWindowInfo;
import Brawler.Monitor;

export namespace Brawler
{
	class WindowedModeWindowState final : public I_WindowState<WindowedModeWindowState>
	{
	public:
		WindowedModeWindowState(AppWindow& owningWnd, const WindowedModeParams& params);

		WindowedModeWindowState(const WindowedModeWindowState& rhs) = delete;
		WindowedModeWindowState& operator=(const WindowedModeWindowState& rhs) = delete;

		WindowedModeWindowState(WindowedModeWindowState&& rhs) noexcept = default;
		WindowedModeWindowState& operator=(WindowedModeWindowState&& rhs) noexcept = default;

		Win32::WindowMessageResult ProcessWindowMessage(const Win32::WindowMessage& msg);
		Win32::CreateWindowInfo GetCreateWindowInfo() const;

		SwapChainCreationInfo GetSwapChainCreationInfo() const;

		void UpdateWindowForDisplayMode();

	private:


		void UpdateWindowRectangle();

	private:
		Math::Int2 mWindowOriginCoordinates;
		Math::UInt2 mWindowSize;
	};
}