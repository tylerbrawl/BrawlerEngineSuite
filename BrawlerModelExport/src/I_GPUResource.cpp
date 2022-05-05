module;
#include <cassert>
#include <optional>
#include "DxDef.h"

module Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.GPUResourceInitializationInfo;
import Util.Engine;

namespace Brawler
{
	namespace D3D12
	{
		I_GPUResource::I_GPUResource(const GPUResourceInitializationInfo& initInfo) :
			mD3D12MAAllocation(nullptr),
			mResource(nullptr),
			mResourceDesc(initInfo.ResourceDesc),
			mCurrState(initInfo.InitialResourceState)
		{
			const std::optional<D3D12_CLEAR_VALUE> optimizedClearValue{ GetOptimizedClearValue() };

			const D3D12_CLEAR_VALUE* optimizedClearValuePtr = nullptr;
			if (optimizedClearValue.has_value()) [[unlikely]]
				optimizedClearValuePtr = &(*optimizedClearValue);

			// Ensure that the initial resource state is consistent with the requirements for the
			// defined heap type.
#ifdef _DEBUG
			if (initInfo.AllocationDesc.HeapType == D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD)
				assert(initInfo.InitialResourceState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ && "ERROR: GPU resources created in an upload heap *MUST* have an initial resource state of D3D12_RESOURCE_STATE_GENERIC_READ!");

			else if (initInfo.AllocationDesc.HeapType == D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK)
				assert(initInfo.InitialResourceState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST && "ERROR: GPU resources created in a readback heap *MUST* have an initial resource state of D3D12_RESOURCE_STATE_COPY_DEST!");
#endif // _DEBUG

			CheckHRESULT(Util::Engine::GetD3D12Allocator().CreateResource2(
				&(initInfo.AllocationDesc),
				&(initInfo.ResourceDesc),
				initInfo.InitialResourceState,
				optimizedClearValuePtr,
				&mD3D12MAAllocation,
				IID_PPV_ARGS(&mResource)
			));
		}

		std::optional<D3D12_CONSTANT_BUFFER_VIEW_DESC> I_GPUResource::CreateCBVDescription() const
		{
			return std::optional<D3D12_CONSTANT_BUFFER_VIEW_DESC>{};
		}

		std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> I_GPUResource::CreateSRVDescription() const
		{
			return std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC>{};
		}

		std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> I_GPUResource::CreateUAVDescription() const
		{
			return std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC>{};
		}

		std::optional<D3D12_CLEAR_VALUE> I_GPUResource::GetOptimizedClearValue() const
		{
			return std::optional<D3D12_CLEAR_VALUE>{};
		}

		Brawler::D3D12Resource& I_GPUResource::GetD3D12Resource()
		{
			return *(mResource.Get());
		}

		const Brawler::D3D12Resource& I_GPUResource::GetD3D12Resource() const
		{
			return *(mResource.Get());
		}

		const Brawler::D3D12_RESOURCE_DESC& I_GPUResource::GetResourceDescription() const
		{
			return mResourceDesc;
		}

		D3D12_RESOURCE_STATES I_GPUResource::GetCurrentResourceState() const
		{
			return mCurrState;
		}

		void I_GPUResource::SetCurrentResourceState(const D3D12_RESOURCE_STATES currentState)
		{
			mCurrState = currentState;
		}
	}
}