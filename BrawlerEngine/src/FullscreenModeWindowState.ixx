module;
#include <DxDef.h>

export module Brawler.FullscreenModeWindowState;
import Brawler.I_WindowState;
import Brawler.Win32.WindowMessage;
import Brawler.Win32.CreateWindowInfo;
import Brawler.SwapChain;

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

		SwapChainCreationInfo GetSwapChainCreationInfo() const;

	private:
		/// <summary>
		/// Retrieves the best possible Brawler::DXGI_MODE_DESC for the Monitor associated with
		/// the AppWindow instance containing this FullscreenModeWindowState instance by comparing
		/// the supported modes to the mode specified within the user configuration file.
		/// 
		/// This is left as a function, rather than cached as a member, because the configuration
		/// file values might change while the program is running.
		/// </summary>
		/// <returns>
		/// The function returns the best possible Brawler::DXGI_MODE_DESC for the Monitor associated 
		/// with the AppWindow instance containing this FullscreenModeWindowState instance by 
		/// comparing the supported modes to the mode specified within the user configuration file.
		/// </returns>
		Brawler::DXGI_MODE_DESC GetBestFullscreenMode() const;
	};
}