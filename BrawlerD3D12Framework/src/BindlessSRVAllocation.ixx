module;
#include <cstdint>
#include <optional>

export module Brawler.D3D12.BindlessSRVAllocation;
import Brawler.D3D12.GPUResourceBindlessSRVManager;

export namespace Brawler
{
	namespace D3D12
	{
		class BindlessSRVAllocation
		{
		public:
			BindlessSRVAllocation() = default;
			BindlessSRVAllocation(GPUResourceBindlessSRVManager& srvManager, BindlessSRVSentinel& associatedSentinel);

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
			/// </summary>
			/// <returns>
			/// The function returns the index into the bindless SRV portion of the descriptor
			/// heap owned by the GPUResourceDescriptorHeap.
			/// </returns>
			std::uint32_t GetBindlessSRVIndex() const;

		private:
			void NotifyManagerToReturnSRV();

		private:
			GPUResourceBindlessSRVManager* mSRVManagerPtr;
			BindlessSRVSentinel* mAssociatedSentinelPtr;
		};
	}
}