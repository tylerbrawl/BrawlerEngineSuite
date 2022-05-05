module;
#include <cassert>
#include "DxDef.h"

export module Brawler.D3D12.RootDescriptors;
import Brawler.D3D12.I_BufferSubAllocation;

namespace Brawler
{
	namespace D3D12
	{
		template <std::uint32_t DummyParam>
		class RootDescriptor
		{
		public:
			explicit RootDescriptor(const I_BufferSubAllocation& bufferSubAllocation);
			explicit RootDescriptor(const D3D12_GPU_VIRTUAL_ADDRESS bufferDataStartAddress);

			RootDescriptor(const RootDescriptor& rhs) = default;
			RootDescriptor& operator=(const RootDescriptor& rhs) = default;

			RootDescriptor(RootDescriptor&& rhs) noexcept = default;
			RootDescriptor& operator=(RootDescriptor&& rhs) noexcept = default;

			D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const;

		private:
			D3D12_GPU_VIRTUAL_ADDRESS mBufferDataStartAddress;
		};
	}
}

// -------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <std::uint32_t DummyParam>
		RootDescriptor<DummyParam>::RootDescriptor(const I_BufferSubAllocation& bufferSubAllocation) :
			mBufferDataStartAddress(bufferSubAllocation.GetGPUVirtualAddress())
		{}

		template <std::uint32_t DummyParam>
		RootDescriptor<DummyParam>::RootDescriptor(const D3D12_GPU_VIRTUAL_ADDRESS bufferDataStartAddress) :
			mBufferDataStartAddress(bufferDataStartAddress)
		{
			assert(bufferDataStartAddress != 0 && "ERROR: A nullptr D3D12_GPU_VIRTUAL_ADDRESS was provided when attempting to create a RootDescriptor! (Is this because you called ID3D12Resource::GetGPUVirtualAddress() for a texture? Textures cannot be used as root descriptors.)");
		}

		template <std::uint32_t DummyParam>
		D3D12_GPU_VIRTUAL_ADDRESS RootDescriptor<DummyParam>::GetGPUVirtualAddress() const
		{
			return mBufferDataStartAddress;
		}
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		using RootConstantBufferView = RootDescriptor<0>;
		using RootShaderResourceView = RootDescriptor<1>;
		using RootUnorderedAccessView = RootDescriptor<2>;
	}
}