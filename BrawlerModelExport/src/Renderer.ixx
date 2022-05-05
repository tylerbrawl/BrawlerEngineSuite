module;
#include <atomic>
#include "DxDef.h"

export module Brawler.D3D12.Renderer;
import Brawler.D3D12.GPUCommandManager;
import Brawler.D3D12.GPUDevice;
import Brawler.D3D12.RootSignatureDatabase;
import Brawler.D3D12.PSODatabase;

export namespace Brawler
{
	namespace D3D12
	{
		class Renderer
		{
		public:
			Renderer() = default;

			Renderer(const Renderer& rhs) = delete;
			Renderer& operator=(const Renderer& rhs) = delete;

			Renderer(Renderer&& rhs) noexcept = default;
			Renderer& operator=(Renderer&& rhs) noexcept = default;

			void Initialize();

			GPUCommandManager& GetGPUCommandManager();
			const GPUCommandManager& GetGPUCommandManager() const;

			GPUDevice& GetGPUDevice();
			const GPUDevice& GetGPUDevice() const;

			D3D12MA::Allocator& GetD3D12Allocator();
			const D3D12MA::Allocator& GetD3D12Allocator() const;

			RootSignatureDatabase& GetRootSignatureDatabase();
			const RootSignatureDatabase& GetRootSignatureDatabase() const;

			PSODatabase& GetPSODatabase();
			const PSODatabase& GetPSODatabase() const;

			void AdvanceFrame();
			std::uint64_t GetCurrentFrameNumber() const;

		private:
			Microsoft::WRL::ComPtr<D3D12MA::Allocator> mD3D12MAAllocator;
			GPUCommandManager mCmdManager;
			GPUDevice mDevice;
			RootSignatureDatabase mRSDatabase;
			PSODatabase mPSODatabase;
			std::atomic<std::uint64_t> mCurrFrameNum;
		};
	}
}