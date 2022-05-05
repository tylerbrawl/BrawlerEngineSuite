module;
#include <cstddef>
#include <cstdint>

export module Brawler.D3D12.UAVCounterSubAllocation;
import Brawler.D3D12.I_BufferSubAllocation;

export namespace Brawler
{
	namespace D3D12
	{
		class UAVCounterSubAllocation final : public I_BufferSubAllocation
		{
		public:
			UAVCounterSubAllocation() = default;

			UAVCounterSubAllocation(const UAVCounterSubAllocation& rhs) = delete;
			UAVCounterSubAllocation& operator=(const UAVCounterSubAllocation& rhs) = delete;

			UAVCounterSubAllocation(UAVCounterSubAllocation&& rhs) noexcept = default;
			UAVCounterSubAllocation& operator=(UAVCounterSubAllocation&& rhs) noexcept = default;

			std::size_t GetSubAllocationSize() const override;
			std::size_t GetRequiredDataPlacementAlignment() const override;

			/// <summary>
			/// Sets the value of this UAV counter from the CPU side to the specified value.
			/// 
			/// *NOTE*: The associated BufferResource *MUST* be created in a D3D12_HEAP_TYPE_UPLOAD
			/// heap for this to work!
			/// </summary>
			/// <param name="counterValue">
			/// - The value to which the UAV counter will be set.
			/// </param>
			void SetCounterValue(const std::uint32_t counterValue) const;

			/// <summary>
			/// Retrieves the current value (on the CPU timeline) of this UAV counter from the 
			/// associated BufferResource.
			/// 
			/// *NOTE*: The associated BufferResource *MUST* be created in a D3D12_HEAP_TYPE_READBACK
			/// heap for this to work!
			/// </summary>
			/// <returns>
			/// The function returns the current value (on the CPU timeline) of this UAV counter from 
			/// the associated BufferResource.
			/// </returns>
			std::uint32_t GetCounterValue() const;
		};
	}
}