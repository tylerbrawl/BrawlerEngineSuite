module;
#include <optional>
#include <DxDef.h>

export module Brawler.SwapChain;

export namespace Brawler
{
	struct SwapChainCreationInfo
	{
		HWND HWnd;
		Brawler::DXGI_SWAP_CHAIN_DESC SwapChainDesc;
		std::optional<DXGI_SWAP_CHAIN_FULLSCREEN_DESC> FullscreenDesc;
	};
	
	class SwapChain
	{
	public:
		SwapChain() = default;

		SwapChain(const SwapChain& rhs) = delete;
		SwapChain& operator=(const SwapChain& rhs) = delete;

		SwapChain(SwapChain&& rhs) noexcept = default;
		SwapChain& operator=(SwapChain&& rhs) noexcept = default;

		void CreateSwapChain(const SwapChainCreationInfo& creationInfo);

	private:
		Microsoft::WRL::ComPtr<Brawler::DXGISwapChain> mSwapChainPtr;
	};
}