module;
#include <DxDef.h>

module Brawler.SwapChain;
import Util.General;
import Util.Engine;
import Brawler.D3D12.PresentationManager;
import Brawler.D3D12.GPUCommandQueue;

namespace Brawler
{
	void SwapChain::CreateSwapChain(const SwapChainCreationInfo& creationInfo)
	{
		const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* const fullscreenDescPtr = (creationInfo.FullscreenDesc.has_value() ? &(*(creationInfo.FullscreenDesc)) : nullptr);

		Microsoft::WRL::ComPtr<IDXGISwapChain1> oldSwapChainPtr{};
		Util::General::CheckHRESULT(Util::Engine::GetDXGIFactory().CreateSwapChainForHwnd(
			&(Util::Engine::GetPresentationManager().GetPresentationCommandQueue().GetD3D12CommandQueue()),
			creationInfo.HWnd,
			&(creationInfo.SwapChainDesc),
			fullscreenDescPtr,
			nullptr,
			&oldSwapChainPtr
		));

		Util::General::CheckHRESULT(oldSwapChainPtr.As(&mSwapChainPtr));

		static constexpr std::uint32_t MAXIMUM_FRAME_LATENCY = static_cast<std::uint32_t>(Util::Engine::MAX_FRAMES_IN_FLIGHT + 1);
		Util::General::CheckHRESULT(mSwapChainPtr->SetMaximumFrameLatency(MAXIMUM_FRAME_LATENCY));

		// Prevent DXGI from handling Alt+Enter fullscreen transitions.
		Util::General::CheckHRESULT(Util::Engine::GetDXGIFactory().MakeWindowAssociation(creationInfo.HWnd, DXGI_MWA_NO_ALT_ENTER));
	}
}