module;

export module Brawler.FullscreenModeWindowState;
import Brawler.I_WindowState;
import Brawler.Win32.WindowMessage;
import Brawler.Win32.CreateWindowInfo;

export namespace Brawler
{
	class FullscreenModeWindowState final : public I_WindowState<FullscreenModeWindowState>
	{
	public:
		explicit FullscreenModeWindowState(AppWindow& owningWnd);

		FullscreenModeWindowState(const FullscreenModeWindowState& rhs) = delete;
		FullscreenModeWindowState& operator=(const FullscreenModeWindowState& rhs) = delete;

		FullscreenModeWindowState(FullscreenModeWindowState&& rhs) noexcept = default;
		FullscreenModeWindowState& operator=(FullscreenModeWindowState&& rhs) noexcept = default;

		Win32::WindowMessageResult ProcessWindowMessage(const Win32::WindowMessage& rhs);
		Win32::CreateWindowInfo GetCreateWindowInfo() const;
	};
}