module;
#include <cstdint>
#include <optional>

export module Brawler.D3D12.GPUResourceDescriptors.BindlessSRVAllocation;

namespace Brawler
{
	namespace D3D12
	{
		class GPUResourceDescriptorHeap;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class BindlessSRVAllocation
		{
		private:
			friend class GPUResourceDescriptorHeap;

		private:
			explicit BindlessSRVAllocation(const std::uint32_t allocationIndex);

		public:
			~BindlessSRVAllocation();

			BindlessSRVAllocation(const BindlessSRVAllocation& rhs) = delete;
			BindlessSRVAllocation& operator=(const BindlessSRVAllocation& rhs) = delete;

			BindlessSRVAllocation(BindlessSRVAllocation&& rhs) noexcept;
			BindlessSRVAllocation& operator=(BindlessSRVAllocation&& rhs) noexcept;

			/// <summary>
			/// Retrieves the index into the bindless SRV portion of the descriptor heap owned
			/// by the GPUResourceDescriptorHeap. The returned value can be used in shaders to
			/// index into descriptor tables which cover the bindless SRV portion of the
			/// descriptor heap.
			/// 
			/// NOTE: This function asserts on Debug builds if it is called after its index
			/// is invalid.
			/// </summary>
			/// <returns>
			/// The function returns the index into the bindless SRV portion of the descriptor
			/// heap owned by the GPUResourceDescriptorHeap.
			/// </returns>
			std::uint32_t GetBindlessSRVIndex() const;

		private:
			void ReClaimAllocation();
			void ResetAllocationIndex();

		private:
			std::optional<std::uint32_t> mAllocationIndex;
		};
	}
}