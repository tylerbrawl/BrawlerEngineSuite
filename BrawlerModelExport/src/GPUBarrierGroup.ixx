module;
#include <vector>
#include <span>
#include "DxDef.h"

export module Brawler.D3D12.GPUBarrierGroup;
import Brawler.D3D12.GPUSplitBarrierToken;
import Brawler.D3D12.GPUResourceHandle;

export namespace Brawler
{
	namespace D3D12
	{
		class GPUBarrierGroup
		{
		public:
			GPUBarrierGroup() = default;

			GPUBarrierGroup(const GPUBarrierGroup& rhs) = delete;
			GPUBarrierGroup& operator=(const GPUBarrierGroup& rhs) = delete;

			GPUBarrierGroup(GPUBarrierGroup&& rhs) noexcept = default;
			GPUBarrierGroup& operator=(GPUBarrierGroup&& rhs) noexcept = default;

			/// <summary>
			/// Adds a resource transition which must be executed by the GPU before additional work
			/// can proceed. This ensures that a resource is in its proper state for whatever its
			/// next use will be.
			/// 
			/// [UNCONFIRMED - PLEASE VERIFY] A resource transition can be used in place of a UAV
			/// barrier if a resource which was previously in the D3D12_RESOURCE_STATE_UNORDERED_ACCESS
			/// state is to be moved to a different state.
			/// 
			/// When possible, try to use the GPUBarrierGroup::BeginSplitResourceTransition() and
			/// GPUBarrierGroup::EndSplitResourceTransition() functions instead. However, if no work
			/// on the GPU can proceed until the resource transition is finished, then calling this
			/// function instead of those two makes more sense.
			/// </summary>
			/// <param name="hResource">
			/// - A handle with write privileges to the resource which is to be transitioned.
			/// </param>
			/// <param name="desiredState">
			/// - The state which the resource specified by hResource will be in once the barrier
			///   is completed.
			/// </param>
			void AddImmediateResourceTransition(const GPUResourceWriteHandle hResource, const D3D12_RESOURCE_STATES desiredState);

			GPUSplitBarrierToken BeginSplitResourceTransition(const GPUResourceWriteHandle hResource, const D3D12_RESOURCE_STATES desiredState);
			void EndSplitResourceTransition(GPUSplitBarrierToken&& splitBarrierToken);

			void AddUAVBarrier(const GPUResourceWriteHandle hResource);

			std::span<const CD3DX12_RESOURCE_BARRIER> GetResourceBarriers() const;

		private:
			std::vector<CD3DX12_RESOURCE_BARRIER> mBarrierArr;
		};
	}
}