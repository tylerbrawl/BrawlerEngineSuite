module;
#include <cassert>
#include "DxDef.h"

module Brawler.D3D12.I_BufferSnapshot;

namespace Brawler
{
	namespace D3D12
	{
		I_BufferSnapshot::I_BufferSnapshot(const I_BufferSubAllocation& bufferSubAllocation) :
			mBufferResourcePtr(&(bufferSubAllocation.GetBufferResource())),
			mSubAllocationSize(bufferSubAllocation.GetSubAllocationSize()),
			mOffsetFromBufferStart(bufferSubAllocation.GetOffsetFromBufferStart())
		{}

		const BufferResource& I_BufferSnapshot::GetBufferResource() const
		{
			assert(mBufferResourcePtr != nullptr);
			return *mBufferResourcePtr;
		}

		Brawler::D3D12Resource& I_BufferSnapshot::GetD3D12Resource() const
		{
			assert(mBufferResourcePtr != nullptr);
			return mBufferResourcePtr->GetD3D12Resource();
		}

		std::size_t I_BufferSnapshot::GetSubAllocationSize() const
		{
			return mSubAllocationSize;
		}

		std::size_t I_BufferSnapshot::GetOffsetFromBufferStart() const
		{
			return mOffsetFromBufferStart;
		}

		D3D12_GPU_VIRTUAL_ADDRESS I_BufferSnapshot::GetGPUVirtualAddress() const
		{
			const D3D12_GPU_VIRTUAL_ADDRESS baseAddress = GetD3D12Resource().GetGPUVirtualAddress();
			assert(baseAddress != 0);

			return baseAddress + GetOffsetFromBufferStart();
		}
	}
}