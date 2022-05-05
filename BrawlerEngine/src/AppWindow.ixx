module;
#include "DxDef.h"

export module Brawler.AppWindow;
import Win32.WindowMessage;
import Brawler.SwapChain;

export namespace Brawler
{
	class AppWindow
	{
	public:
		AppWindow();

		void InitializeMainWindow();

		HWND GetWindowHandle() const;
		void ProcessIncomingWindowMessages();
		Win32::WindowMessageResult ProcessWindowMessage(const Win32::WindowMessage& msg);

		Brawler::DXGIOutput& GetCurrentDXGIOutput();
		const Brawler::DXGIOutput& GetCurrentDXGIOutput() const;
		std::uint32_t GetMonitorRefreshRate() const;

	private:
		void RegisterWindowClass() const;
		void CreateWin32Window();
		void UpdateCurrentDXGIOutput();

	private:
		HWND mHWnd;
		Microsoft::WRL::ComPtr<Brawler::DXGIOutput> mCurrOutput;
		SwapChain mSwapChain;
	};
}