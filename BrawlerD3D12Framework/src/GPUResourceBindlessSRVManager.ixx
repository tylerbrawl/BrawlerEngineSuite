module;
#include <memory>
#include "DxDef.h"

export module Brawler.D3D12.GPUResourceBindlessSRVManager;
import Brawler.ThreadSafeVector;
import Brawler.OptionalRef;
import Brawler.D3D12.BindlessSRVSentinel;
import Brawler.D3D12.BindlessGPUResourceGroupRegistration;

export namespace Brawler
{
	namespace D3D12
	{
		class BindlessSRVAllocation;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class GPUResourceBindlessSRVManager
		{
		public:
			GPUResourceBindlessSRVManager() = default;
			~GPUResourceBindlessSRVManager();

			GPUResourceBindlessSRVManager(const GPUResourceBindlessSRVManager& rhs) = delete;
			GPUResourceBindlessSRVManager& operator=(const GPUResourceBindlessSRVManager& rhs) = delete;

			GPUResourceBindlessSRVManager(GPUResourceBindlessSRVManager&& rhs) noexcept = default;
			GPUResourceBindlessSRVManager& operator=(GPUResourceBindlessSRVManager&& rhs) noexcept = default;

			BindlessSRVAllocation CreateBindlessSRV(D3D12_SHADER_RESOURCE_VIEW_DESC&& srvDesc, Brawler::OptionalRef<Brawler::D3D12Resource> d3dResource);
			void ReturnBindlessSRV(BindlessSRVSentinel& sentinel);

			void UpdateBindlessSRVs(Brawler::D3D12Resource& d3dResource);

			void AddBindlessGPUResourceGroupAssociation(BindlessGPUResourceGroupRegistration& groupRegistration);
			void RemoveBindlessGPUResourceGroupAssociation(BindlessGPUResourceGroupRegistration& groupRegistration);

		private:
			Brawler::ThreadSafeVector<std::unique_ptr<BindlessSRVSentinel>> mBindlessSentinelArr;
			Brawler::ThreadSafeVector<BindlessGPUResourceGroupRegistration*> mGroupRegistrationPtrArr;
		};
	}
}