module;
#include <cassert>
#include <span>
#include <vector>
#include <DxDef.h>

module Brawler.GenericPreFrameBufferUpdate;

namespace Brawler
{
	GenericPreFrameBufferUpdate::GenericPreFrameBufferUpdate(GenericPreFrameBufferUpdateInfo&& updateInfo) :
		mUpdateInfo(std::move(updateInfo)),
		mDataByteArr()
	{
		assert(mUpdateInfo.DestBufferResourcePtr != nullptr);
		assert(mUpdateInfo.OffsetFromBufferStart + mUpdateInfo.UpdateRegionSizeInBytes <= mUpdateInfo.DestBufferResourcePtr->GetResourceDescription().Width);

		mDataByteArr.resize(mUpdateInfo.UpdateRegionSizeInBytes);
	}

	D3D12::BufferResource& GenericPreFrameBufferUpdate::GetBufferResource()
	{
		assert(mUpdateInfo.DestBufferResourcePtr != nullptr);
		return *(mUpdateInfo.DestBufferResourcePtr);
	}

	D3D12::BufferCopyRegion GenericPreFrameBufferUpdate::GetBufferCopyRegion() const
	{
		return D3D12::BufferCopyRegion{ D3D12::BufferCopyRegionInfo{
			.BufferResourcePtr = mUpdateInfo.DestBufferResourcePtr,
			.OffsetFromBufferStart = mUpdateInfo.OffsetFromBufferStart,
			.RegionSizeInBytes = mUpdateInfo.UpdateRegionSizeInBytes
		} };
	}

	std::span<const std::byte> GenericPreFrameBufferUpdate::GetDataByteSpan() const
	{
		return { mDataByteArr };
	}

	std::vector<std::byte> GenericPreFrameBufferUpdate::ExtractDataByteArray()
	{
		return std::move(mDataByteArr);
	}

	std::size_t GenericPreFrameBufferUpdate::GetUpdateRegionSize() const
	{
		assert(mUpdateInfo.UpdateRegionSizeInBytes == mDataByteArr.size());
		return mUpdateInfo.UpdateRegionSizeInBytes;
	}
}