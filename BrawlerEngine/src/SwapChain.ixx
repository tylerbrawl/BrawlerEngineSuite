module;
#include "DxDef.h"

export module Brawler.SwapChain;

export namespace Brawler
{
	class AppWindow;
}

export namespace Brawler
{
	class SwapChain
	{
	public:
		enum class DisplayCurve
		{
			SRGB,  // Rec. 709
			HDR,   // Rec. 2020
			
			UNDEFINED
		};

	public:
		explicit SwapChain(AppWindow& owningWnd);

		void Initialize();

	private:
		void UpdateDisplayCurve();
		void CreateSwapChain();
		void UpdateSwapChainColorSpace();
		DXGI_SWAP_CHAIN_DESC1 CreateSwapChainDescription() const;

	private:
		AppWindow* const mOwningWnd;
		Microsoft::WRL::ComPtr<Brawler::DXGISwapChain> mSwapChain;
		DisplayCurve mDisplayCurve;
	};
}