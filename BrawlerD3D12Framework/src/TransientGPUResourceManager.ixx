module;
#include <vector>
#include <memory>
#include <span>

export module Brawler.D3D12.TransientGPUResourceManager;
import Brawler.D3D12.I_GPUResourceHeapManager;
import Brawler.D3D12.I_GPUResource;

export namespace Brawler
{
	namespace D3D12
	{
		class TransientGPUResourceManager
		{
		public:
			TransientGPUResourceManager() = default;

			TransientGPUResourceManager(const TransientGPUResourceManager& rhs) = delete;
			TransientGPUResourceManager& operator=(const TransientGPUResourceManager& rhs) = delete;

			TransientGPUResourceManager(TransientGPUResourceManager&& rhs) noexcept = default;
			TransientGPUResourceManager& operator=(TransientGPUResourceManager&& rhs) noexcept = default;

			void Initialize();

			void AddTransientResources(const std::span<std::unique_ptr<I_GPUResource>> transientResourceSpan);
			void DeleteTransientResources();

			I_GPUResourceHeapManager<GPUResourceLifetimeType::TRANSIENT>& GetGPUResourceHeapManager();
			const I_GPUResourceHeapManager<GPUResourceLifetimeType::TRANSIENT>& GetGPUResourceHeapManager() const;

		private:
			/// <summary>
			/// This is the I_GPUResourceHeapManager instance which contains all of the
			/// GPUResourceHeaps for transient resources used by the FrameGraph which
			/// owns this TransientGPUResourceManager instance.
			/// 
			/// This implies that the lifetime of transient resources in a FrameGraph - 
			/// both on the CPU and on the GPU - is managed by the FrameGraph's
			/// TransientGPUResourceManager.
			/// </summary>
			std::unique_ptr<I_GPUResourceHeapManager<GPUResourceLifetimeType::TRANSIENT>> mTransientResourceHeapManager;

			std::vector<std::unique_ptr<I_GPUResource>> mTransientResourceArr;
		};
	}
}