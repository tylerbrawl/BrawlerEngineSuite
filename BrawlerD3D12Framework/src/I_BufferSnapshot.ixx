module;
#include "DxDef.h"

export module Brawler.D3D12.I_BufferSnapshot;
import Brawler.D3D12.I_BufferSubAllocation;
import Brawler.D3D12.BufferResource;

export namespace Brawler
{
	namespace D3D12
	{
		class I_BufferSnapshot
		{
		protected:
			explicit I_BufferSnapshot(const I_BufferSubAllocation& bufferSubAllocation);

		public:
			virtual ~I_BufferSnapshot() = default;

			I_BufferSnapshot(const I_BufferSnapshot& rhs) = default;
			I_BufferSnapshot& operator=(const I_BufferSnapshot& rhs) = default;

			I_BufferSnapshot(I_BufferSnapshot&& rhs) noexcept = default;
			I_BufferSnapshot& operator=(I_BufferSnapshot&& rhs) noexcept = default;

			const BufferResource& GetBufferResource() const;

			Brawler::D3D12Resource& GetD3D12Resource() const;

			std::size_t GetSubAllocationSize() const;
			std::size_t GetOffsetFromBufferStart() const;

			D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const;

		private:
			const BufferResource* mBufferResourcePtr;
			std::size_t mSubAllocationSize;
			std::size_t mOffsetFromBufferStart;
		};
	}
}