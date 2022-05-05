module;
#include <optional>
#include "DxDef.h"

module Brawler.D3D12.PlacedD3D12ResourceContainer;
import Brawler.D3D12.GPUResourceInitializationInfo;
import Brawler.D3D12.I_GPUResource;
import Util.Engine;

namespace Brawler
{
	namespace D3D12
	{
		PlacedD3D12ResourceContainer::PlacedD3D12ResourceContainer(I_GPUResource& owningResource) :
			D3D12ResourceContainer(owningResource),
			mHAllocation()
		{}

		void PlacedD3D12ResourceContainer::CreatePlacedD3D12Resource(GPUResourceAllocationHandle&& hAllocation, const GPUResourceInitializationInfo& initInfo)
		{
			const std::optional<D3D12_CLEAR_VALUE> optimizedClearValue{ GetGPUResource().GetOptimizedClearValue() };
			const D3D12_CLEAR_VALUE* optimizedClearValuePtr = nullptr;

			// Buffers cannot specify an optimized clear value.
			if (initInfo.ResourceDesc.Dimension != D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER && optimizedClearValue.has_value()) [[unlikely]]
				optimizedClearValuePtr = &(*optimizedClearValue);

			Microsoft::WRL::ComPtr<Brawler::D3D12Resource> d3dResource{};
			CheckHRESULT(Util::Engine::GetD3D12Device().CreatePlacedResource1(
				&(hAllocation.GetD3D12Heap()),
				hAllocation.GetHeapOffset(),
				&(initInfo.ResourceDesc),

				// Other threads may be modifying the value stored in the I_GPUResource's
				// mCurrState while the actual resource is being allocated on the GPU. So, 
				// we need to pass the InitialResourceState member of its mInitInfo to 
				// prevent a data race. (From a contextual standpoint, using this also better
				// aligns with what the CreatePlacedResource1() function is asking for: the 
				// resource's initial resource state.)
				initInfo.InitialResourceState,

				optimizedClearValuePtr,
				IID_PPV_ARGS(&d3dResource)
			));

			mHAllocation = std::move(hAllocation);
			SetD3D12Resource(std::move(d3dResource));
		}
	}
}