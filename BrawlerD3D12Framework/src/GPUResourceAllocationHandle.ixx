module;
#include <cstdint>
#include "DxDef.h"

export module Brawler.D3D12.GPUResourceAllocationHandle;

export namespace Brawler
{
	namespace D3D12
	{
		class GPUResourceAllocation;
		class I_GPUResource;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class GPUResourceAllocationHandle
		{
		private:
			friend class GPUResourceAllocation;

		public:
			GPUResourceAllocationHandle() = default;
			GPUResourceAllocationHandle(GPUResourceAllocation& owningAllocation, const I_GPUResource& resource);

			~GPUResourceAllocationHandle();

			GPUResourceAllocationHandle(const GPUResourceAllocationHandle& rhs) = delete;
			GPUResourceAllocationHandle& operator=(const GPUResourceAllocationHandle& rhs) = delete;

			GPUResourceAllocationHandle(GPUResourceAllocationHandle&& rhs) noexcept;
			GPUResourceAllocationHandle& operator=(GPUResourceAllocationHandle&& rhs) noexcept;

			Brawler::D3D12Heap& GetD3D12Heap() const;
			std::size_t GetHeapOffset() const;

		private:
			GPUResourceAllocation* mOwningAllocationPtr;
			const I_GPUResource* mResourcePtr;
		};
	}
}