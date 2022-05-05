module;
#include <memory>
#include <atomic>
#include "DxDef.h"

export module Brawler.D3D12.PersistentGPUResourceManager;
import Brawler.D3D12.I_GPUResourceHeapManager;
import Brawler.D3D12.GPUResourceLifetimeType;
import Brawler.Win32.SafeHandle;
import Brawler.ThreadSafeVector;

export namespace Brawler
{
	namespace D3D12
	{
		class I_GPUResource;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class PersistentGPUResourceManager
		{
		public:
			PersistentGPUResourceManager() = default;

			PersistentGPUResourceManager(const PersistentGPUResourceManager& rhs) = delete;
			PersistentGPUResourceManager& operator=(const PersistentGPUResourceManager& rhs) = delete;

			PersistentGPUResourceManager(PersistentGPUResourceManager&& rhs) noexcept = default;
			PersistentGPUResourceManager& operator=(PersistentGPUResourceManager&& rhs) noexcept = default;

			void Initialize();

			HRESULT AllocatePersistentGPUResource(I_GPUResource& resource);

		private:
			void CreateGPUResourceHeapManager();

		private:
			std::unique_ptr<I_GPUResourceHeapManager<GPUResourceLifetimeType::PERSISTENT>> mHeapManager;
		};
	}
}