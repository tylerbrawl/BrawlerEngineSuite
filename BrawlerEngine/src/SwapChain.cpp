module;
#include "DxDef.h"

module Brawler.SwapChain;
import Brawler.AppWindow;
import Util.Engine;
import Brawler.SettingID;
import Brawler.Renderer;
import Brawler.CommandQueue;
import Brawler.CommandListType;

namespace
{
	// According to NVIDIA, we should use 1-2 more back buffers than frames which are queued.
	static constexpr std::uint32_t BACK_BUFFER_COUNT = 3;
	
	DXGI_FORMAT GetBackBufferFormatForDisplayCurve(const Brawler::SwapChain::DisplayCurve dispCurve)
	{
		switch (dispCurve)
		{
		case Brawler::SwapChain::DisplayCurve::SRGB:
			return DXGI_FORMAT_R8G8B8A8_UNORM;

		case Brawler::SwapChain::DisplayCurve::HDR:
			return DXGI_FORMAT_R10G10B10A2_UNORM;  // Fun Fact: The documentation doesn't state that this is supported for flip-mode swap chains, but it is!

		default:
			// What do we do here? Should we just use a default format, or end the program?
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		}
	}

	DXGI_COLOR_SPACE_TYPE GetColorSpaceTypeForDisplayCurve(const Brawler::SwapChain::DisplayCurve dispCurve)
	{
		switch (dispCurve)
		{
		case Brawler::SwapChain::DisplayCurve::SRGB:
			return DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;

		case Brawler::SwapChain::DisplayCurve::HDR:
			return DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;

		default:
			return DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
		}
	}
}

namespace Brawler
{
	SwapChain::SwapChain(AppWindow& owningWnd) :
		mOwningWnd(&owningWnd),
		mSwapChain(nullptr),
		mDisplayCurve(DisplayCurve::UNDEFINED)
	{}

	void SwapChain::Initialize()
	{
		UpdateDisplayCurve();
		CreateSwapChain();
		UpdateSwapChainColorSpace();
	}

	void SwapChain::UpdateDisplayCurve()
	{
		DXGI_OUTPUT_DESC1 currOutputDesc{};
		CheckHRESULT(mOwningWnd->GetCurrentDXGIOutput().GetDesc1(&currOutputDesc));

		switch (currOutputDesc.ColorSpace)
		{
		case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709:
		{
			mDisplayCurve = DisplayCurve::SRGB;
			return;
		}

		case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020:
		{
			mDisplayCurve = DisplayCurve::HDR;
			return;
		}

		default:
		{
			// According to the MSDN at https://docs.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range#desktop-win32-directx-apps,
			// this should never happen. If it does, however, then we're screwed...

			mDisplayCurve = DisplayCurve::UNDEFINED;
			return;
		}
		}
	}

	void SwapChain::CreateSwapChain()
	{
		const DXGI_SWAP_CHAIN_DESC1 scDesc{ CreateSwapChainDescription() };

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc{};
		fullscreenDesc.RefreshRate.Numerator = mOwningWnd->GetMonitorRefreshRate();
		fullscreenDesc.RefreshRate.Denominator = 1;
		fullscreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		fullscreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		fullscreenDesc.Windowed = !(Util::Engine::GetOption<Brawler::SettingID::USE_FULLSCREEN>());

		Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain{ nullptr };
		CheckHRESULT(Util::Engine::GetDXGIFactory().CreateSwapChainForHwnd(
			&(Util::Engine::GetCommandQueue(Brawler::CommandListType::DIRECT).GetD3D12CommandQueue()),
			mOwningWnd->GetWindowHandle(),
			&scDesc,
			&fullscreenDesc,
			nullptr,
			&swapChain
		));

		CheckHRESULT(swapChain.As(&mSwapChain));
		CheckHRESULT(mSwapChain->SetMaximumFrameLatency(BACK_BUFFER_COUNT));
	}

	void SwapChain::UpdateSwapChainColorSpace()
	{
		// Ensure that the correct color space is set.
		DXGI_OUTPUT_DESC1 currOutputDesc{};
		CheckHRESULT(mOwningWnd->GetCurrentDXGIOutput().GetDesc1(&currOutputDesc));
		CheckHRESULT(mSwapChain->SetColorSpace1(currOutputDesc.ColorSpace));
	}

	DXGI_SWAP_CHAIN_DESC1 SwapChain::CreateSwapChainDescription() const
	{
		DXGI_SWAP_CHAIN_DESC1 scDesc{};

		scDesc.Width = static_cast<std::uint32_t>(Util::Engine::GetOption<Brawler::SettingID::WINDOW_RESOLUTION_WIDTH>() *
			Util::Engine::GetOption<Brawler::SettingID::RENDER_RESOLUTION_FACTOR>());

		scDesc.Height = static_cast<std::uint32_t>(Util::Engine::GetOption<Brawler::SettingID::WINDOW_RESOLUTION_HEIGHT>() *
			Util::Engine::GetOption<Brawler::SettingID::RENDER_RESOLUTION_FACTOR>());

		scDesc.Format = GetBackBufferFormatForDisplayCurve(mDisplayCurve);

		// TODO: What the hell is this? Is this for things like VR headsets?
		scDesc.Stereo = FALSE;

		scDesc.SampleDesc.Count = 1;
		scDesc.SampleDesc.Quality = 0;

		scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scDesc.BufferCount = BACK_BUFFER_COUNT;
		scDesc.Scaling = DXGI_SCALING_STRETCH;
		scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		scDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		scDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

		// Check for tearing support.
		BOOL isTearingSupported = false;
		CheckHRESULT(Util::Engine::GetDXGIFactory().CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &isTearingSupported, sizeof(isTearingSupported)));

		if (isTearingSupported)
			scDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

		return scDesc;
	}
}