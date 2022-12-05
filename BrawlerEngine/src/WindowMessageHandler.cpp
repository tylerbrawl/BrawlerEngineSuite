#define OEMRESOURCE

module;
#include <cassert>
#include <stdexcept>
#include <unordered_map>
#include <DxDef.h>

module Brawler.WindowMessageHandler;
import Brawler.Application;
import Brawler.Manifest;
import Brawler.NZStringView;
import Util.General;
import Util.Threading;
import Brawler.Win32.WindowMessage;

namespace Brawler
{
	WindowMessageHandler::WindowMessageHandler() :
		mHWndMap()
	{
		RegisterWindowClass();
	}

	WindowMessageHandler& WindowMessageHandler::GetInstance()
	{
		static WindowMessageHandler instance{};
		return instance;
	}

	LRESULT WindowMessageHandler::WndProc(
		const HWND hWnd,
		const UINT uMsg,
		const WPARAM wParam,
		const LPARAM lParam
	)
	{
		if (!mHWndMap.contains(hWnd)) [[unlikely]]
		{
			return DefWindowProc(
				hWnd,
				uMsg,
				wParam,
				lParam
			);
		}
			
		AppWindow& appWindow{ *(mHWndMap.at(hWnd)) };
		const Win32::WindowMessage windowMsg{
			.Msg = uMsg,
			.WParam = wParam,
			.LParam = lParam
		};

		const Win32::WindowMessageResult result{ appWindow.ProcessWindowMessage(windowMsg) };

		// If this is a WM_DESTROY message, then the associated Win32 window is being
		// destroyed. In that case, we can remove it from mHWndMap.
		if (uMsg == WM_DESTROY) [[unlikely]]
			mHWndMap.erase(hWnd);

		return (result.MessageHandled ? result.Result : DefWindowProc(hWnd, uMsg, wParam, lParam));
	}

	void WindowMessageHandler::RegisterWindow(AppWindow& window)
	{
		// Technically speaking, we *could* allow multiple threads to call this function
		// by using a thread-safe map. Since the rest of the functionality requires the
		// main thread be used, however, this doesn't seem worth it.
		assert(Util::Threading::IsMainThread() && "ERROR: Only the main thread can call functions within WindowMessageHandler! (It is assumed that only the main thread will handle Win32 windows and window messages. This is an artifact of the Win32 window message system, since only the thread which creates a window is able to retrieve its window messages.)");
		
		const HWND hWnd = window.GetWindowHandle();
		mHWndMap[hWnd] = &window;
	}

	void WindowMessageHandler::DispatchWindowMessages()
	{
		assert(Util::Thread::IsMainThread() && "ERROR: Only the main thread can dispatch Win32 window messages! (It is assumed that only the main thread will handle Win32 windows and window messages. This is an artifact of the Win32 window message system, since only the thread which creates a window is able to retrieve its window messages.)");
		
		MSG currMsg{};

		while (true)
		{
			const BOOL peekMessageResult = PeekMessage(
				&currMsg,
				nullptr,
				0,
				0,
				PM_REMOVE
			);

			if (!peekMessageResult)
				break;

			TranslateMessage(&currMsg);
			DispatchMessage(&currMsg);
		}
	}

	void WindowMessageHandler::RegisterWindowClass()
	{
		static constexpr WNDCLASSEX DEFAULT_WINDOW_CLASS_VALUE{
			.cbSize = sizeof(WNDCLASSEX),
			.style = CS_DROPSHADOW,
			.lpfnWndProc = WndProc,
			.lpszClassName = Brawler::Manifest::WINDOW_CLASS_NAME_STR.C_Str()
		};

		WNDCLASSEX wndClass{ DEFAULT_WINDOW_CLASS_VALUE };
		wndClass.hInstance = Brawler::GetApplication().GetInstanceHandle();

		// TODO: Allow for the application to use a different HICON to change how it appears in
		// the taskbar, among other places.

		wndClass.hCursor = reinterpret_cast<HCURSOR>(LoadImage(
			nullptr,
			MAKEINTRESOURCE(OCR_NORMAL),
			IMAGE_CURSOR,
			0,
			0,
			LR_DEFAULTSIZE | LR_SHARED
		));

		const ATOM registerClassResult = RegisterClassEx(&wndClass);

		if (registerClassResult == 0) [[unlikely]]
			throw std::runtime_error{ "ERROR: The application's window class could not be registered!" };
	}
}