#define OEMRESOURCE

module;
#include "DxDef.h"

module Brawler.AppWindow;
import Brawler.Application;
import Brawler.Manifest;
import Util.General;
import Util.Engine;
import Brawler.SettingsManager;
import Brawler.NZStringView;

namespace
{
	static constexpr Brawler::NZWStringView APPLICATION_WINDOW_CLASS_NAME{ L"BrawlerEngineWindowClass::Main" };
	static Brawler::AppWindow* windowInstance = nullptr;

	LRESULT WINAPI WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		assert(windowInstance != nullptr && "ERROR: A window message was received before the static Brawler::AppWindow pointer could be initialized!");

		Win32::WindowMessageResult result{ windowInstance->ProcessWindowMessage(Win32::WindowMessage{uMsg, wParam, lParam}) };
		return (result.MessageHandled ? result.Result : DefWindowProc(hWnd, uMsg, wParam, lParam));
	}
}

namespace Brawler
{
	AppWindow::AppWindow() :
		mHWnd(nullptr),
		mCurrOutput(nullptr),
		mSwapChain(*this)
	{
		assert(windowInstance == nullptr && "ERROR: An attempt was made to create a second Brawler::AppWindow instance!");
		windowInstance = this;
	}

	void AppWindow::InitializeMainWindow()
	{
		RegisterWindowClass();
		CreateWin32Window();

		UpdateCurrentDXGIOutput();

		// Prevent Alt + Enter from entering fullscreen.
		Util::Engine::GetDXGIFactory().MakeWindowAssociation(mHWnd, DXGI_MWA_NO_ALT_ENTER);

		mSwapChain.Initialize();
	}

	HWND AppWindow::GetWindowHandle() const
	{
		return mHWnd;
	}

	void AppWindow::ProcessIncomingWindowMessages()
	{
		MSG msg{};

		while (PeekMessage(&msg, mHWnd, 0, 0, PM_REMOVE | PM_QS_INPUT | PM_QS_SENDMESSAGE | PM_QS_POSTMESSAGE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	Win32::WindowMessageResult AppWindow::ProcessWindowMessage(const Win32::WindowMessage& msg)
	{
		switch (msg.Msg)
		{
		case WM_EXITSIZEMOVE: [[fallthrough]];
		case WM_DISPLAYCHANGE:
		{
			UpdateCurrentDXGIOutput();

			return Win32::HandledMessageResult(0);
		}

		case WM_DESTROY:
		{
			Brawler::GetApplication().Terminate();

			return Win32::HandledMessageResult(0);
		}

		default:
			return Win32::UnhandledMessageResult();
		}
	}

	Brawler::DXGIOutput& AppWindow::GetCurrentDXGIOutput()
	{
		return *(mCurrOutput.Get());
	}

	const Brawler::DXGIOutput& AppWindow::GetCurrentDXGIOutput() const
	{
		return *(mCurrOutput.Get());
	}

	std::uint32_t AppWindow::GetMonitorRefreshRate() const
	{
		// Under DX12, there is no such thing as exclusive full-screen mode; *ALL* windows are
		// handled by the DWM.
		//
		// Whether or not this is a good thing is not for me to decide. However, it does mean
		// that we can get the current refresh rate by querying the DWM timing info.

		DWM_TIMING_INFO timingInfo{};
		timingInfo.cbSize = sizeof(timingInfo);

		Util::General::CheckHRESULT(DwmGetCompositionTimingInfo(nullptr, &timingInfo));
		return timingInfo.rateRefresh.uiNumerator / timingInfo.rateRefresh.uiDenominator;
	}

	void AppWindow::RegisterWindowClass() const
	{
		WNDCLASSEX wndClass{};
		wndClass.cbSize = sizeof(wndClass);
		wndClass.style = CS_HREDRAW | CS_VREDRAW;
		wndClass.lpfnWndProc = WndProc;
		wndClass.hInstance = Brawler::GetApplication().GetInstanceHandle();

		// Load the default cursor. Since this is an OEM resource loaded with LR_SHARED, we
		// do not delete the handle.
		HCURSOR hCursor = reinterpret_cast<HCURSOR>(LoadImage(
			nullptr,
			MAKEINTRESOURCE(OCR_NORMAL),
			IMAGE_CURSOR,
			0,
			0,
			LR_SHARED | LR_DEFAULTSIZE
		));
		wndClass.hCursor = hCursor;

		wndClass.lpszClassName = APPLICATION_WINDOW_CLASS_NAME.C_Str();

		RegisterClassEx(&wndClass);
	}

	void AppWindow::CreateWin32Window()
	{
		mHWnd = CreateWindowEx(
			WS_EX_OVERLAPPEDWINDOW,
			APPLICATION_WINDOW_CLASS_NAME,
			Brawler::Manifest::APPLICATION_NAME.C_Str(),
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			Brawler::SettingsManager::GetInstance().GetOption<SettingID::WINDOW_RESOLUTION_WIDTH>(),
			Brawler::SettingsManager::GetInstance().GetOption<SettingID::WINDOW_RESOLUTION_HEIGHT>(),
			nullptr,
			nullptr,
			Brawler::GetApplication().GetInstanceHandle(),
			nullptr
		);

		// ShowWindow(mHWnd, nCmdShow);
	}

	void AppWindow::UpdateCurrentDXGIOutput()
	{
		Microsoft::WRL::ComPtr<IDXGIOutput> currOutput{};
		HMONITOR hCurrMonitor = MonitorFromWindow(mHWnd, MONITOR_DEFAULTTOPRIMARY);

		std::uint32_t currIndex = 0;
		while (SUCCEEDED(Util::Engine::GetDXGIAdapter().EnumOutputs(currIndex++, &currOutput)))
		{
			DXGI_OUTPUT_DESC currOutputDesc{};
			Util::General::CheckHRESULT(currOutput->GetDesc(&currOutputDesc));

			if (hCurrMonitor == currOutputDesc.Monitor)
				break;
		}

		Util::General::CheckHRESULT(currOutput.As(&mCurrOutput));
	}
}