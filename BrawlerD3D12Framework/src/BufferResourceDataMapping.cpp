module;
#include <cstddef>
#include <cassert>
#include <span>
#include "DxDef.h"

module Brawler.D3D12.BufferResourceDataMapping;

namespace Brawler
{
	namespace D3D12
	{
		UploadBufferResourceDataMapping::UploadBufferResourceDataMapping(const I_BufferSubAllocation& bufferSubAllocation) :
			BufferResourceDataMapping<DataMappingType::WRITE_ONLY>(bufferSubAllocation)
		{}

		void UploadBufferResourceDataMapping::WriteDataToBuffer(const std::span<const std::byte> srcDataSpan, const std::size_t offsetFromSubAllocationStart) const
		{
			const std::span<std::byte> destDataSpan{ GetMappedData() };
			assert(offsetFromSubAllocationStart + srcDataSpan.size_bytes() <= destDataSpan.size_bytes() && "ERROR: An attempt was made to write outside of the bounds of a buffer sub-allocation with an UploadBufferResourceDataMapping()!");

			std::memcpy(destDataSpan.data() + offsetFromSubAllocationStart, srcDataSpan.data(), srcDataSpan.size_bytes());
		}
	}
}

namespace Brawler
{
	namespace D3D12
	{
		ReadBackBufferResourceDataMapping::ReadBackBufferResourceDataMapping(const I_BufferSubAllocation& bufferSubAllocation) :
			BufferResourceDataMapping<DataMappingType::READ_ONLY>(bufferSubAllocation)
		{}

		std::span<const std::byte> ReadBackBufferResourceDataMapping::GetReadBackDataSpan() const
		{
			return GetMappedData();
		}
	}
}