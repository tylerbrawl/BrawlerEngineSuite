module;
#include <atomic>

export module Brawler.D3D12.Renderer;
import Brawler.D3D12.GPUCommandManager;
import Brawler.D3D12.GPUDevice;
import Brawler.D3D12.PersistentGPUResourceManager;
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

			template <typename RSIdentifierEnumType, typename PSOIdentifierEnumType>
			void Initialize();

			GPUCommandManager& GetGPUCommandManager();
			const GPUCommandManager& GetGPUCommandManager() const;

			GPUDevice& GetGPUDevice();
			const GPUDevice& GetGPUDevice() const;

			PersistentGPUResourceManager& GetPersistentGPUResourceManager();
			const PersistentGPUResourceManager& GetPersistentGPUResourceManager() const;

			void AdvanceFrame();
			std::uint64_t GetCurrentFrameNumber() const;

		private:
			GPUDevice mDevice;
			GPUCommandManager mCmdManager;
			PersistentGPUResourceManager mPersistentResourceManager;
			std::atomic<std::uint64_t> mCurrFrameNum;
		};
	}
}

// ---------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <typename RSIdentifierEnumType, typename PSOIdentifierEnumType>
		void Renderer::Initialize()
		{
			// Initialize the GPUDevice.
			mDevice.Initialize();

			// Initialize the PersistentGPUResourceManager.
			mPersistentResourceManager.Initialize();

			// Initialize the GPUCommandManager.
			mCmdManager.Initialize();

			// Initialize the RootSignatureDatabase.
			RootSignatureDatabase<RSIdentifierEnumType>::GetInstance();

			// Initialize the PSODatabase. This *MUST* be initialized after the RootSignatureDatabase,
			// since PSO compilation relies on compiled root signatures.
			PSODatabase<PSOIdentifierEnumType>::GetInstance();
		}
	}
}