module;
#include "DxDef.h"

export module Brawler.D3D12.CommittedD3D12ResourceContainer;
import Brawler.D3D12.D3D12ResourceContainer;
import Brawler.D3D12.I_PageableGPUObject;

export namespace Brawler
{
	namespace D3D12
	{
		struct GPUResourceInitializationInfo;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class CommittedD3D12ResourceContainer final : public D3D12ResourceContainer, public I_PageableGPUObject
		{
		public:
			explicit CommittedD3D12ResourceContainer(I_GPUResource& owningResource);

			void CreateCommittedD3D12Resource(const D3D12_HEAP_FLAGS heapFlags, const GPUResourceInitializationInfo& initInfo);

		private:
			bool NeedsResidency() const override;
			float GetUsageMetric() const override;
			std::size_t GetGPUMemorySize() const override;
			ID3D12Pageable& GetD3D12PageableObject() const override;
			bool IsDeletionSafe() const override;
			void DeleteD3D12PageableObject() override;
		};
	}
}