module;
#include <optional>
#include <DxDef.h>

export module Brawler.BorderlessWindowedModeWindowState;
import Brawler.I_WindowState;
import Brawler.Win32.WindowMessage;
import Brawler.Win32.CreateWindowInfo;
import Brawler.SwapChain;

export namespace Brawler
{
	class BorderlessWindowedModeWindowState final : public I_WindowState<BorderlessWindowedModeWindowState>
	{
	public:
		explicit BorderlessWindowedModeWindowState(AppWindow& owningWnd);

		~BorderlessWindowedModeWindowState();

		BorderlessWindowedModeWindowState(const BorderlessWindowedModeWindowState& rhs) = delete;
		BorderlessWindowedModeWindowState& operator=(const BorderlessWindowedModeWindowState& rhs) = delete;

		BorderlessWindowedModeWindowState(BorderlessWindowedModeWindowState&& rhs) noexcept = default;
		BorderlessWindowedModeWindowState& operator=(BorderlessWindowedModeWindowState&& rhs) noexcept = default;

		Win32::WindowMessageResult ProcessWindowMessage(const Win32::WindowMessage& msg);
		void OnShowWindow();

		Win32::CreateWindowInfo GetCreateWindowInfo() const;
		SwapChainCreationInfo GetSwapChainCreationInfo() const;

	private:
		/// <summary>
		/// Attempts to retrieve the HWND representing the Windows Taskbar for the monitor on 
		/// which the AppWindow currently in this BorderlessWindowedModeWindowState is situated. 
		/// The taskbar HWND is searched programmatically.
		/// 
		/// Keep in mind that the nature of this search is inherently hacky in nature. While
		/// it is unlikely to fail in any versions of Windows we know of any time soon, we'll
		/// need to keep up with changes in future versions of the OS. If the function fails
		/// to find the taskbar HWND, then the returned std::optional instance is empty.
		/// 
		/// NOTE: The taskbar HWND is not cached because the user is allowed to change its
		/// position to one of four pre-defined locations at any time through the Windows
		/// Settings, and we currently have no known way of detecting this. As future work, we
		/// can investigate the WM_SETTINGCHANGE window message.
		/// </summary>
		/// <returns>
		/// If the HWND representing the Windows Taskbar for the monitor on which the AppWindow
		/// currently in this BorderlessWindowedModeWindowState is situated could be located,
		/// then the returned std::optional instance has a value, and this value is the
		/// HWND of said taskbar.
		/// 
		/// Otherwise, the returned std::optional instance has no value.
		/// </returns>
		std::optional<HWND> GetTaskbarWindowForCurrentMonitor() const;

		void ShowTaskbar() const;
		void HideTaskbar() const;
	};
}