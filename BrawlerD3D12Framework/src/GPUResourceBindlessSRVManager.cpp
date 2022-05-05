module;
#include <cassert>
#include "DxDef.h"

module Brawler.D3D12.GPUResourceBindlessSRVManager;
import Util.Engine;
import Brawler.D3D12.GPUResourceDescriptorHeap;
import Brawler.D3D12.BindlessSRVAllocation;

namespace Brawler
{
	namespace D3D12
	{
		GPUResourceBindlessSRVManager::~GPUResourceBindlessSRVManager()
		{
			assert(mBindlessSentinelArr.Empty() && "ERROR: A GPUResourceBindlessSRVManager was destroyed before all of the BindlessSRVAllocation instances which it created were destroyed!");
		}

		BindlessSRVAllocation GPUResourceBindlessSRVManager::CreateBindlessSRV(D3D12_SHADER_RESOURCE_VIEW_DESC&& srvDesc, Brawler::OptionalRef<Brawler::D3D12Resource> d3dResource)
		{
			std::unique_ptr<BindlessSRVSentinel> bindlessSentinel{ Util::Engine::GetGPUResourceDescriptorHeap().AllocateBindlessSRV() };
			bindlessSentinel->SetSRVDescription(std::move(srvDesc));

			if (d3dResource.HasValue())
				bindlessSentinel->UpdateBindlessSRV(*d3dResource);

			BindlessSRVSentinel* const createdBindlessSentinelPtr = bindlessSentinel.get();
			mBindlessSentinelArr.PushBack(std::move(bindlessSentinel));

			return BindlessSRVAllocation{ *this, *createdBindlessSentinelPtr };
		}

		void GPUResourceBindlessSRVManager::ReturnBindlessSRV(BindlessSRVSentinel& sentinel)
		{
			mBindlessSentinelArr.EraseIf([&sentinel] (const std::unique_ptr<BindlessSRVSentinel>& existingSentinel) { return (&sentinel == existingSentinel.get()); });
		}

		void GPUResourceBindlessSRVManager::UpdateBindlessSRVs(Brawler::D3D12Resource& d3dResource)
		{
			mBindlessSentinelArr.ForEach([&d3dResource] (const std::unique_ptr<BindlessSRVSentinel>& sentinel)
			{
				sentinel->UpdateBindlessSRV(d3dResource);
			});
		}
	}
}