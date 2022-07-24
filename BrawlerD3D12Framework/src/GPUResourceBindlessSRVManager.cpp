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

			// Remove any associations made with BindlessGPUResourceGroup instances. This will prevent additional
			// FrameGraphResourceDependencies from being made for it.
			//
			// Regarding the lifetime of BindlessGPUResourceGroupRegistration instances, it is assumed that the
			// BindlessGPUResourceGroup instance which owns them will outlive the I_GPUResource instances which
			// register with the group.
			//
			// There is also the possibility that a resource might be destroyed while the BindlessGPUResourceGroup
			// instance is checking for FrameGraphResourceDependency instances to create. This typically happens
			// during RenderPass creation, and if a resource which is in use is being destroyed at that point in
			// time, then that was already an error to begin with. Even without a BindlessGPUResourceGroupRegistration,
			// if a destroyed I_GPUResource instance is referenced during FrameGraph building, then that was already
			// going to invoke undefined behavior.
			mGroupRegistrationPtrArr.ForEach([] (BindlessGPUResourceGroupRegistration* const& registrationPtr)
			{
				registrationPtr->MarkAsInvalidated();
			});
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

		void GPUResourceBindlessSRVManager::AddBindlessGPUResourceGroupAssociation(BindlessGPUResourceGroupRegistration& groupRegistration)
		{
			mGroupRegistrationPtrArr.PushBack(&groupRegistration);
		}

		void GPUResourceBindlessSRVManager::RemoveBindlessGPUResourceGroupAssociation(BindlessGPUResourceGroupRegistration& groupRegistration)
		{
			mGroupRegistrationPtrArr.EraseIf([registrationPtr = &groupRegistration] (BindlessGPUResourceGroupRegistration* const& existingRegistrationPtr)
			{
				if (existingRegistrationPtr != registrationPtr)
					return false;

				registrationPtr->MarkAsInvalidated();
				return true;
			});
		}
	}
}