module;
#include <DxDef.h>

module Brawler.I_WindowState;
import Brawler.AppWindow;
import Brawler.Monitor;
import Util.Engine;

namespace Brawler
{
	Brawler::DXGI_SWAP_CHAIN_DESC GetDefaultSwapChainDescription(const AppWindow& window)
	{
		// It is recommended by NVIDIA to create 1-2 more back buffers than you have frames in
		// flight.
		constexpr std::uint32_t SWAP_CHAIN_BUFFER_COUNT = (Util::Engine::MAX_FRAMES_IN_FLIGHT + 1);

		// NOTE: The description of the DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH flag on the MSDN at
		// https://docs.microsoft.com/en-us/windows/win32/api/dxgi/ne-dxgi-dxgi_swap_chain_flag#constants
		// is rather misleading, in my opinion. It gives the impression that if the flag is set, then
		// calling IDXGISwapChain::ResizeTarget() may cause the swap chain to transition from fullscreen 
		// to windowed mode and vice versa.
		//
		// When the description says that the flag enables the application to "switch modes by calling
		// IDXGISwapChain::ResizeTarget()," it is NOT stating that calling this function might result
		// in the swap chain moving from windowed to fullscreen mode or vice versa. Rather, it means
		// that calling this function with the swap chain in fullscreen mode might result in the desktop
		// resolution (or display *mode*) being changed.
		//
		// They try to clarify this in the description's second statement, but the ambiguous use of the
		// term "modes" in the first statement led me to believe that said "modes" were actually referring
		// to the swap between fullscreen and windowed mode. Anyways, I'm just leaving this here in case
		// anybody else was as confused by that as I was.
		DXGI_SWAP_CHAIN_FLAG swapChainFlags = static_cast<DXGI_SWAP_CHAIN_FLAG>(DXGI_SWAP_CHAIN_FLAG::DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT | DXGI_SWAP_CHAIN_FLAG::DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);

		if (Util::Engine::GetGPUCapabilities().IsTearingAllowed) [[likely]]
			swapChainFlags = static_cast<DXGI_SWAP_CHAIN_FLAG>(swapChainFlags | DXGI_SWAP_CHAIN_FLAG::DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);

		return Brawler::DXGI_SWAP_CHAIN_DESC{
			.Width{},
			.Height{},
			.Format = window.GetOwningMonitor().GetPreferredSwapChainFormat(),
			.Stereo = FALSE,
			.SampleDesc{
				.Count = 1,
				.Quality = 0
			},
			.BufferUsage = (DXGI_CPU_ACCESS_NONE | DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT),
			.BufferCount = SWAP_CHAIN_BUFFER_COUNT,

			// We always want the swap chain buffer size to be the same as the target output dimensions.
			// According to the D3D12 samples, this is needed in order to get access to true independent
			// flip mode in fullscreen.
			//
			// Even for a windowed mode swap chain description, we still want to do this. That
			// way, we can have a uniform scaling method for all window modes.
			.Scaling = DXGI_SCALING::DXGI_SCALING_NONE,

			.SwapEffect = DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_FLIP_DISCARD,
			.AlphaMode = DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_UNSPECIFIED,
			.Flags = static_cast<std::uint32_t>(swapChainFlags)
		};
	}
}