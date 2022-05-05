module;
#include <atomic>
#include <cassert>
#include <optional>
#include <mutex>
#include "DxDef.h"

module Brawler.I_GPUResource;
import Brawler.D3DHeapAllocationHandle;
import Brawler.ResourceDescriptorHeapAllocation;
import Util.Engine;
import Brawler.Renderer;
import Brawler.ResourceDescriptorHeap;

namespace Brawler
{
	I_GPUResource::I_GPUResource() :
		mD3DResource(nullptr),
		mProperlyInitialized(false),
		mHAllocation(),
		mDescriptorManager(*this),
		mBindlessSRVAllocation(),
		mBindlessSRVAllocationCritSection(),
		mCurrState(D3D12_RESOURCE_STATE_COMMON)
	{}

	I_GPUResource::~I_GPUResource()
	{}

	std::optional<D3D12_CLEAR_VALUE> I_GPUResource::GetOptimizedClearValue() const
	{
		return std::optional<D3D12_CLEAR_VALUE>{};
	}

	std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> I_GPUResource::GetSRVDescription() const
	{
		return std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC>{};
	}

	std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> I_GPUResource::GetUAVDescription() const
	{
		const std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> srvDesc{ GetSRVDescription() };

		if (!srvDesc.has_value())
			return std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC>{};

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
		uavDesc.Format = srvDesc->Format;

		switch (srvDesc->ViewDimension)
		{
		case D3D12_SRV_DIMENSION_BUFFER:
		{
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uavDesc.Buffer.FirstElement = srvDesc->Buffer.FirstElement;
			uavDesc.Buffer.NumElements = srvDesc->Buffer.NumElements;
			uavDesc.Buffer.StructureByteStride = srvDesc->Buffer.StructureByteStride;

			// If your UAV needs a counter, then override this function.
			//
			// NOTE: Counters must be placed at an offset which is a multiple of 
			// D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT.
			uavDesc.Buffer.CounterOffsetInBytes = 0;

			if (srvDesc->Buffer.Flags & D3D12_BUFFER_SRV_FLAG_RAW)
				uavDesc.Buffer.Flags |= D3D12_BUFFER_UAV_FLAG_RAW;

			break;
		}

		case D3D12_SRV_DIMENSION_TEXTURE1D:
		{
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
			uavDesc.Texture1D.MipSlice = srvDesc->Texture1D.MostDetailedMip;
			
			break;
		}

		case D3D12_SRV_DIMENSION_TEXTURE1DARRAY:
		{
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
			uavDesc.Texture1DArray.MipSlice = srvDesc->Texture1DArray.MostDetailedMip;
			uavDesc.Texture1DArray.FirstArraySlice = srvDesc->Texture1DArray.FirstArraySlice;
			uavDesc.Texture1DArray.ArraySize = srvDesc->Texture1DArray.ArraySize;

			break;
		}

		case D3D12_SRV_DIMENSION_TEXTURE2D:
		{
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			uavDesc.Texture2D.MipSlice = srvDesc->Texture2D.MostDetailedMip;
			uavDesc.Texture2D.PlaneSlice = srvDesc->Texture2D.PlaneSlice;
			
			break;
		}

		case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
		{
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
			uavDesc.Texture2DArray.MipSlice = srvDesc->Texture2DArray.MostDetailedMip;
			uavDesc.Texture2DArray.FirstArraySlice = srvDesc->Texture2DArray.FirstArraySlice;
			uavDesc.Texture2DArray.ArraySize = srvDesc->Texture2DArray.ArraySize;
			uavDesc.Texture2DArray.PlaneSlice = srvDesc->Texture2DArray.PlaneSlice;
			
			break;
		}

		// We can't get enough information from the D3D12_SHADER_RESOURCE_VIEW_DESC
		// alone for 3D textures.
		case D3D12_SRV_DIMENSION_TEXTURE3D:
		default:
		{
			assert(false && "ERROR: An attempt was made to create a D3D12_UNORDERED_ACCESS_VIEW_DESC for an I_GPUResource from its D3D12_SHADER_RESOURCE_VIEW_DESC, but this could not be done! If calling I_GPUResource::GetUAVDescription() for this resource was intentional, then provide an override for the function to manually create one.");
			return std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC>{};
		}
		}

		return std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC>{ std::move(uavDesc) };
	}

	Brawler::D3D12Resource& I_GPUResource::GetD3D12Resource()
	{
		assert(mProperlyInitialized && "ERROR: An attempt was made to create and access a resource which was not initialized with a function from Brawler::Renderer!");
		
		assert(mD3DResource != nullptr);
		return *(mD3DResource.Get());
	}

	const Brawler::D3D12Resource& I_GPUResource::GetD3D12Resource() const
	{
		assert(mProperlyInitialized && "ERROR: An attempt was made to create and access a resource which was not initialized with a function from Brawler::Renderer!");
		
		assert(mD3DResource != nullptr);
		return *(mD3DResource.Get());
	}

	D3D12_RESOURCE_STATES I_GPUResource::GetCurrentResourceState() const
	{
		return mCurrState;
	}

	void I_GPUResource::CreateBindlessSRV()
	{
		std::scoped_lock<std::mutex> lock{ mBindlessSRVAllocationCritSection };
		Util::Engine::GetRenderer().GetResourceDescriptorHeap().CreateBindlessSRV(*this);
	}

	void I_GPUResource::InitializeIMPL(Microsoft::WRL::ComPtr<Brawler::D3D12Resource>&& d3dResource, D3DHeapAllocationHandle&& hAllocation, const D3D12_RESOURCE_STATES currentState)
	{
		mD3DResource = std::move(d3dResource);
		mHAllocation = std::move(hAllocation);
		mCurrState = currentState;

		mDescriptorManager.Initialize();
	}

	void I_GPUResource::SetCurrentResourceState(const D3D12_RESOURCE_STATES currentState)
	{
		mCurrState = currentState;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE I_GPUResource::GetCPUVisibleDescriptorHandle(const ResourceDescriptorType descriptorType) const
	{
		return mDescriptorManager.GetCPUVisibleDescriptorHandle(descriptorType);
	}

	void I_GPUResource::SetBindlessSRVAllocation(ResourceDescriptorHeapAllocation&& bindlessAllocation)
	{
		mBindlessSRVAllocation = std::move(bindlessAllocation);
	}
}