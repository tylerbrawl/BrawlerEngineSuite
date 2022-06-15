module;
#include <vector>
#include <span>
#include "DxDef.h"

export module Brawler.D3D12.GPUSubResourceStateManager;

export namespace Brawler
{
	namespace D3D12
	{
		class GPUSubResourceStateManager
		{
		public:
			GPUSubResourceStateManager(const D3D12_RESOURCE_STATES initialState, const std::uint32_t subResourceCount);

			GPUSubResourceStateManager(const GPUSubResourceStateManager& rhs) = delete;
			GPUSubResourceStateManager& operator=(const GPUSubResourceStateManager& rhs) = delete;

			GPUSubResourceStateManager(GPUSubResourceStateManager&& rhs) noexcept = default;
			GPUSubResourceStateManager& operator=(GPUSubResourceStateManager&& rhs) noexcept = default;

			/// <summary>
			/// If subResourceIndex == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, then every sub-resource
			/// for the associated I_GPUResource has its current resource state set to state. Otherwise,
			/// only the sub-resource identified by subResourceIndex has its resource state changed to
			/// that of state.
			/// </summary>
			/// <param name="state">
			/// - The D3D12_RESOURCE_STATES value to which either a single sub-resource or every
			///   sub-resource is transitioned to.
			/// </param>
			/// <param name="subResourceIndex">
			/// - The index of the sub-resource whose state is to be transitioned. If this value is set
			///   to D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, then every sub-resource is set to the
			///   specified state.
			/// </param>
			void SetSubResourceState(const D3D12_RESOURCE_STATES state, const std::uint32_t subResourceIndex);

			D3D12_RESOURCE_STATES GetSubResourceState(const std::uint32_t subResourceIndex) const;
			std::span<const D3D12_RESOURCE_STATES> GetAllSubResourceStates() const;

		private:
			std::vector<D3D12_RESOURCE_STATES> mSubResourceStateArr;
		};
	}
}