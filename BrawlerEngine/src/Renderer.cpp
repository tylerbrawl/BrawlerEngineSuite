module;
#include "DxDef.h"

module Brawler.Renderer;

namespace Brawler
{
	Renderer::Renderer() :
		mDXGIFactory(nullptr),
		mDispAdapter(nullptr),
		mHeapManager(),
		mResourceDescriptorHeap(),
		mRenderJobManager(),
		mCurrFrameNum(0),
		mInitialized(false)
	{}

	void Renderer::Initialize()
	{
		// In Debug builds, we will enable the DirectX 12 debug layer.
#ifdef _DEBUG
		{
			Microsoft::WRL::ComPtr<ID3D12Debug3> debugController{};
			HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
			debugController->EnableDebugLayer();

			// We should also enable GPU-based validation layers.
			debugController->SetEnableGPUBasedValidation(true);
		}

		std::uint32_t factoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#else
		std::uint32_t factoryFlags = 0;
#endif // _DEBUG

		// Create the DXGI factory.
		CheckHRESULT(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&mDXGIFactory)));

		// Create the D3D12Device for the best possible display adapter. We will
		// only bother using one. (Besides, who uses multiple GPUs nowadays?)
		mDispAdapter = GetBestDisplayAdapter();
		mDispAdapter.Initialize();

		mResourceDescriptorHeap.Initialize();

		mRenderJobManager.Initialize();

		mInitialized = true;
	}

	DisplayAdapter Renderer::GetBestDisplayAdapter() const
	{
		// With the later versions of Windows 10, we can use IDXGIFactory6::EnumAdapterByGpuPreference()
		// to easily select the best adapter.

		Microsoft::WRL::ComPtr<Brawler::DXGIAdapter> bestAdapter{};
		std::uint32_t currIndex = 0;

		while (SUCCEEDED(mDXGIFactory->EnumAdapterByGpuPreference(currIndex++, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&bestAdapter))))
		{
			DisplayAdapter potentialAdapter{ std::move(bestAdapter) };

			if (potentialAdapter.IsSupportedAdapter())
				return potentialAdapter;
		}

		// If DirectX 12 fails to find any supported adapters, then we throw an exception.
		throw std::runtime_error{ "IDXGIFactory::EnumAdapters() failed to find a single video card with display capabilities!" };
	}

	DisplayAdapter& Renderer::GetDisplayAdapter()
	{
		return mDispAdapter;
	}

	const DisplayAdapter& Renderer::GetDisplayAdapter() const
	{
		return mDispAdapter;
	}

	Brawler::DXGIFactory& Renderer::GetDXGIFactory()
	{
		return *(mDXGIFactory.Get());
	}

	const Brawler::DXGIFactory& Renderer::GetDXGIFactory() const
	{
		return *(mDXGIFactory.Get());
	}

	Brawler::ResourceDescriptorHeap& Renderer::GetResourceDescriptorHeap()
	{
		return mResourceDescriptorHeap;
	}

	const Brawler::ResourceDescriptorHeap& Renderer::GetResourceDescriptorHeap() const
	{
		return mResourceDescriptorHeap;
	}

	Brawler::RenderJobManager& Renderer::GetRenderJobManager()
	{
		return mRenderJobManager;
	}

	const Brawler::RenderJobManager& Renderer::GetRenderJobManager() const
	{
		return mRenderJobManager;
	}

	Brawler::PSOManager& Renderer::GetPSOManager()
	{
		return mPSOManager;
	}

	const Brawler::PSOManager& Renderer::GetPSOManager() const
	{
		return mPSOManager;
	}

	std::uint64_t Renderer::GetCurrentFrameNumber() const
	{
		return mCurrFrameNum;
	}

	void Renderer::AdvanceFrame()
	{
		++mCurrFrameNum;

		// TODO: Complete work which must be done every frame (e.g., deleting old frame descriptors,
		// evicting unused D3DHeaps, etc.).
		mResourceDescriptorHeap.AdvanceFrame();
	}
}