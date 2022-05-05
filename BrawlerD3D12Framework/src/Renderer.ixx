module;
#include <atomic>
#include "DxDef.h"

export module Brawler.D3D12.Renderer;
import Brawler.D3D12.GPUCommandManager;
import Brawler.D3D12.GPUDevice;
import Brawler.D3D12.RootSignatureDatabase;
import Brawler.D3D12.PSODatabase;
import Brawler.D3D12.PersistentGPUResourceManager;

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

			RootSignatureDatabase& GetRootSignatureDatabase();
			const RootSignatureDatabase& GetRootSignatureDatabase() const;

			PSODatabase& GetPSODatabase();
			const PSODatabase& GetPSODatabase() const;

			PersistentGPUResourceManager& GetPersistentGPUResourceManager();
			const PersistentGPUResourceManager& GetPersistentGPUResourceManager() const;

			void AdvanceFrame();
			std::uint64_t GetCurrentFrameNumber() const;

		private:
			GPUDevice mDevice;
			GPUCommandManager mCmdManager;
			RootSignatureDatabase mRSDatabase;
			PSODatabase mPSODatabase;
			PersistentGPUResourceManager mPersistentResourceManager;
			std::atomic<std::uint64_t> mCurrFrameNum;
		};
	}
}