module;
#include <cassert>
#include <optional>
#include "DxDef.h"

module Brawler.D3D12.ComputeContext;

namespace Brawler
{
	namespace D3D12
	{
		void ComputeContext::RecordCommandListIMPL(const std::function<void(ComputeContext&)>& recordJob)
		{
			recordJob(*this);
		}

		void ComputeContext::PrepareCommandListIMPL()
		{
			mCurrPSOID.reset();
		}

		void ComputeContext::Dispatch(const std::uint32_t numThreadGroupsX, const std::uint32_t numThreadGroupsY, const std::uint32_t numThreadGroupsZ) const
		{
			assert(numThreadGroupsX <= D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION && "ERROR: Too many thread groups were dispatched in the X-dimension in a call to ComputeContext::Dispatch()!");
			assert(numThreadGroupsY <= D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION && "ERROR: Too many thread groups were dispatched in the Y-dimension in a call to ComputeContext::Dispatch()!");
			assert(numThreadGroupsZ <= D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION && "ERROR: Too many thread groups were dispatched in the Z-dimension in a call to ComputeContext::Dispatch()!");

			GetCommandList().Dispatch(numThreadGroupsX, numThreadGroupsY, numThreadGroupsZ);
		}

		void ComputeContext::Dispatch1D(const std::uint32_t numThreadGroups) const
		{
			Dispatch(numThreadGroups, 1, 1);
		}

		void ComputeContext::Dispatch2D(const std::uint32_t numThreadGroupsX, const std::uint32_t numThreadGroupsY) const
		{
			Dispatch(numThreadGroupsX, numThreadGroupsY, 1);
		}

		void ComputeContext::Dispatch3D(const std::uint32_t numThreadGroupsX, const std::uint32_t numThreadGroupsY, const std::uint32_t numThreadGroupsZ) const
		{
			Dispatch(numThreadGroupsX, numThreadGroupsY, numThreadGroupsZ);
		}
	}
}