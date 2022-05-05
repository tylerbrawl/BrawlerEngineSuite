module;
#include <memory>
#include <cassert>
#include "DxDef.h"

export module Brawler.Renderer;
import Brawler.DisplayAdapter;
import Brawler.D3DHeapManager;
import Brawler.ResourceCreationInfo;
import Brawler.I_GPUResource;
import Brawler.RenderJobManager;
import Brawler.ResourceDescriptorHeap;
import Brawler.PSOManager;

export namespace Brawler
{
	class Renderer
	{
	public:
		Renderer();

		Renderer(const Renderer& rhs) = delete;
		Renderer& operator=(const Renderer& rhs) = delete;

		Renderer(Renderer&& rhs) noexcept = default;
		Renderer& operator=(Renderer&& rhs) noexcept = default;

		void Initialize();

		template <typename T, typename... Args>
			requires std::derived_from<T, I_GPUResource>
		std::unique_ptr<T> CreateDefaultResource(const D3D12_RESOURCE_STATES initialState, Args&&... args);

		template <typename T, typename... Args>
			requires std::derived_from<T, I_GPUResource>
		std::unique_ptr<T> CreateCPUWriteableResource(Args&&... args);

		template <typename T, typename... Args>
			requires std::derived_from<T, I_GPUResource>
		std::unique_ptr<T> CreateCPUReadableResource(Args&&... args);

	private:
		DisplayAdapter GetBestDisplayAdapter() const;

	public:
		DisplayAdapter& GetDisplayAdapter();
		const DisplayAdapter& GetDisplayAdapter() const;

		Brawler::DXGIFactory& GetDXGIFactory();
		const Brawler::DXGIFactory& GetDXGIFactory() const;

		ResourceDescriptorHeap& GetResourceDescriptorHeap();
		const ResourceDescriptorHeap& GetResourceDescriptorHeap() const;

		RenderJobManager& GetRenderJobManager();
		const RenderJobManager& GetRenderJobManager() const;

		PSOManager& GetPSOManager();
		const PSOManager& GetPSOManager() const;

		std::uint64_t GetCurrentFrameNumber() const;

		/// <summary>
		/// This function handles changes which must occur after a frame has been drawn by the Brawler
		/// Engine.
		/// </summary>
		void AdvanceFrame();

	private:
		Microsoft::WRL::ComPtr<IDXGIFactory7> mDXGIFactory;
		DisplayAdapter mDispAdapter;
		PSOManager mPSOManager;
		D3DHeapManager mHeapManager;
		ResourceDescriptorHeap mResourceDescriptorHeap;
		RenderJobManager mRenderJobManager;
		std::uint64_t mCurrFrameNum;
		bool mInitialized;
	};
}

// ---------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename T, typename... Args>
		requires std::derived_from<T, I_GPUResource>
	std::unique_ptr<T> Renderer::CreateDefaultResource(const D3D12_RESOURCE_STATES initialState, Args&&... args)
	{
		assert(mInitialized);
		return mHeapManager.CreateResource<T>(D3DResourceAccessMode::NO_CPU_ACCESS, initialState, std::forward<Args>(args)...);
	}

	template <typename T, typename... Args>
		requires std::derived_from<T, I_GPUResource>
	std::unique_ptr<T> Renderer::CreateCPUWriteableResource(Args&&... args)
	{
		assert(mInitialized);
		return mHeapManager.CreateResource<T>(D3DResourceAccessMode::CPU_WRITE_ONLY, D3D12_RESOURCE_STATE_GENERIC_READ, std::forward<Args>(args)...);
	}

	template <typename T, typename... Args>
		requires std::derived_from<T, I_GPUResource>
	std::unique_ptr<T> Renderer::CreateCPUReadableResource(Args&&... args)
	{
		assert(mInitialized);
		return mHeapManager.CreateResource<T>(D3DResourceAccessMode::CPU_READ_ONLY, D3D12_RESOURCE_STATE_COPY_DEST, std::forward<Args>(args)...);
	}
}