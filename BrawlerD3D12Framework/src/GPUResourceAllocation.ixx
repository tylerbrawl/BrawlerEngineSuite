module;
#include <mutex>
#include <vector>
#include "DxDef.h"

export module Brawler.D3D12.GPUResourceAllocation;
import Brawler.D3D12.GPUResourceLifetimeType;
import Brawler.ThreadSafeVector;

export namespace Brawler
{
	namespace D3D12
	{
		class TLSFMemoryBlock;
		class I_GPUResource;
	}
}

namespace Brawler
{
	namespace D3D12
	{
		template <D3D12_HEAP_TYPE HeapType, GPUResourceLifetimeType LifetimeType>
		class GPUResourceHeap;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class GPUResourceAllocation
		{
		private:
			template <D3D12_HEAP_TYPE HeapType, GPUResourceLifetimeType LifetimeType>
			friend class GPUResourceHeap;

		private:
			struct GPUResourceInfo
			{
				I_GPUResource* GPUResourcePtr;

				mutable std::mutex CritSection;
			};

		public:
			GPUResourceAllocation() = default;

			GPUResourceAllocation(const GPUResourceAllocation& rhs) = delete;
			GPUResourceAllocation& operator=(const GPUResourceAllocation& rhs) = delete;

			GPUResourceAllocation(GPUResourceAllocation&& rhs) noexcept = default;
			GPUResourceAllocation& operator=(GPUResourceAllocation&& rhs) noexcept = default;

			/// <summary>
			/// Assigns an I_GPUResource to this GPUResourceAllocation instance, creating a
			/// new ID3D12Resource object for said I_GPUResource and invalidating any old
			/// GPUResourceAllocation assignment which it may have had.
			/// </summary>
			/// <param name="resource">
			/// - The I_GPUResource instance which is to be assigned to this GPUResourceAllocation
			///   instance.
			/// </param>
			void AssignGPUResource(I_GPUResource& resource);

			void RevokeGPUResourceAssignment(const I_GPUResource& resource);

			Brawler::D3D12Heap& GetD3D12Heap() const;
			std::size_t GetHeapOffset() const;

			TLSFMemoryBlock& GetTLSFBlock();
			const TLSFMemoryBlock& GetTLSFBlock() const;

			bool HasGPUResourceAssignments() const;

			bool CouldAllocatedResourcesBeEvicted() const;
			float GetMaximumGPUResourceUsageMetric() const;

		private:
			void SetD3D12Heap(Brawler::D3D12Heap& d3dHeap);
			void SetTLSFBlock(TLSFMemoryBlock& block);
			void ResetTLSFBlock();

		private:
			Brawler::D3D12Heap* mD3DHeapPtr;
			TLSFMemoryBlock* mAllocatedBlockPtr;
			Brawler::ThreadSafeVector<std::unique_ptr<GPUResourceInfo>> mResourceInfoArr;
		};
	}
}