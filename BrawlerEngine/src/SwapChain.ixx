module;
#include <DxDef.h>

export module Brawler.SwapChain;

export namespace Brawler
{
	class SwapChain
	{
	public:
		SwapChain() = default;

		SwapChain(const SwapChain& rhs) = delete;
		SwapChain& operator=(const SwapChain& rhs) = delete;

		SwapChain(SwapChain&& rhs) noexcept = default;
		SwapChain& operator=(SwapChain&& rhs) noexcept = default;



	private:
		Microsoft::WRL::ComPtr<Brawler::DXGISwapChain> mSwapChainPtr;
	};
}