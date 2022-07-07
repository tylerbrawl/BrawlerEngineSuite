module;

export module Brawler.BorderlessWindowedModeWindowState;
import Brawler.I_WindowState;
import Brawler.Win32.WindowMessage;
import Brawler.Win32.CreateWindowInfo;

export namespace Brawler
{
	class BorderlessWindowedModeWindowState final : public I_WindowState<BorderlessWindowedModeWindowState>
	{
	public:
		explicit BorderlessWindowedModeWindowState(AppWindow& owningWnd);

		BorderlessWindowedModeWindowState(const BorderlessWindowedModeWindowState& rhs) = delete;
		BorderlessWindowedModeWindowState& operator=(const BorderlessWindowedModeWindowState& rhs) = delete;

		BorderlessWindowedModeWindowState(BorderlessWindowedModeWindowState&& rhs) noexcept = default;
		BorderlessWindowedModeWindowState& operator=(BorderlessWindowedModeWindowState&& rhs) noexcept = default;

		Win32::WindowMessageResult ProcessWindowMessage(const Win32::WindowMessage& msg);
		Win32::CreateWindowInfo GetCreateWindowInfo() const;
	};
}