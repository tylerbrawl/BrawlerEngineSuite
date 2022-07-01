module;
#include <cstddef>
#include <utility>
#include <cassert>
#include "DxDef.h"

module Brawler.D3D12.BufferCopyRegion;

namespace Brawler
{
	namespace D3D12
	{
		BufferCopyRegion::BufferCopyRegion(BufferCopyRegionInfo&& regionInfo) :
			mRegionInfo(std::move(regionInfo))
		{}

		const BufferResource& BufferCopyRegion::GetBufferResource() const
		{
			assert(mRegionInfo.BufferResourcePtr != nullptr);
			return *(mRegionInfo.BufferResourcePtr);
		}

		Brawler::D3D12Resource& BufferCopyRegion::GetD3D12Resource() const
		{
			assert(mRegionInfo.BufferResourcePtr != nullptr);
			return mRegionInfo.BufferResourcePtr->GetD3D12Resource();
		}

		std::size_t BufferCopyRegion::GetOffsetFromBufferStart() const
		{
			return mRegionInfo.OffsetFromBufferStart;
		}

		std::size_t BufferCopyRegion::GetCopyRegionSize() const
		{
			return mRegionInfo.RegionSizeInBytes;
		}
	}
}