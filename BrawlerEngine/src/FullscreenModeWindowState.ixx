module;
#include <DxDef.h>

export module Brawler.FullscreenModeWindowState;
import Brawler.I_WindowState;
import Brawler.Win32.WindowMessage;
import Brawler.Win32.CreateWindowInfo;
import Brawler.SwapChain;
import Brawler.FullscreenModeParams;

export namespace Brawler
{
	class FullscreenModeWindowState final : public I_WindowState<FullscreenModeWindowState>
	{
	public:
		FullscreenModeWindowState(AppWindow& owningWnd, const FullscreenModeParams& params);

		FullscreenModeWindowState(const FullscreenModeWindowState& rhs) = delete;
		FullscreenModeWindowState& operator=(const FullscreenModeWindowState& rhs) = delete;

		FullscreenModeWindowState(FullscreenModeWindowState&& rhs) noexcept = default;
		FullscreenModeWindowState& operator=(FullscreenModeWindowState&& rhs) noexcept = default;

		Win32::WindowMessageResult ProcessWindowMessage(const Win32::WindowMessage& rhs);
		Win32::CreateWindowInfo GetCreateWindowInfo() const;

		SwapChainCreationInfo GetSwapChainCreationInfo() const;

		void UpdateWindowForDisplayMode();

	private:
		static Brawler::DXGI_MODE_DESC GetBestFullscreenMode(const FullscreenModeParams& params);

	private:
		const Monitor* mMonitorPtr;
		Brawler::DXGI_MODE_DESC mBestFullscreenModeDesc;
	};
}