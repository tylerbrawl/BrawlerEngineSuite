module;
#include <unordered_map>
#include <DxDef.h>

export module Brawler.WindowMessageHandler;
import Brawler.Win32.CreateWindowInfo;
import Brawler.AppWindow;

export namespace Brawler
{
	class WindowMessageHandler
	{
	private:
		WindowMessageHandler();

	public:
		~WindowMessageHandler() = default;

		WindowMessageHandler(const WindowMessageHandler& rhs) = delete;
		WindowMessageHandler& operator=(const WindowMessageHandler& rhs) = delete;

		WindowMessageHandler(WindowMessageHandler&& rhs) noexcept = delete;
		WindowMessageHandler& operator=(WindowMessageHandler&& rhs) noexcept = delete;

		static WindowMessageHandler& GetInstance();

		static LRESULT WndProc(
			const HWND hWnd,
			const UINT uMsg,
			const WPARAM wParam,
			const LPARAM lParam
		);

		void RegisterWindow(AppWindow& window);
		void DispatchWindowMessages();

	private:
		void RegisterWindowClass();

	private:
		std::unordered_map<HWND, AppWindow*> mHWndMap;
	};
}