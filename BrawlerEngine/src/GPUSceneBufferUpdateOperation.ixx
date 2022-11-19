module;
#include <vector>
#include <cstdint>
#include <span>
#include <algorithm>
#include <cassert>
#include <vector>

export module Brawler.GPUSceneBufferUpdateOperation;
import Brawler.GPUSceneBufferID;
import Brawler.GPUSceneBufferMap;
import Brawler.D3D12.BufferCopyRegion;

export namespace Brawler
{
	template <GPUSceneBufferID BufferID>
	class GPUSceneBufferUpdateOperation
	{
	private:
		using DataType = std::decay_t<GPUSceneBufferElementType<BufferID>>;

	public:
		GPUSceneBufferUpdateOperation() = default;
		explicit GPUSceneBufferUpdateOperation(const D3D12::BufferCopyRegion& destCopyRegion);

		GPUSceneBufferUpdateOperation(const GPUSceneBufferUpdateOperation& rhs) = delete;
		GPUSceneBufferUpdateOperation& operator=(const GPUSceneBufferUpdateOperation& rhs) = delete;

		GPUSceneBufferUpdateOperation(GPUSceneBufferUpdateOperation&& rhs) noexcept = default;
		GPUSceneBufferUpdateOperation& operator=(GPUSceneBufferUpdateOperation&& rhs) noexcept = default;

		void SetUpdateSourceData(const std::span<const DataType> dataSpan);

		const D3D12::BufferCopyRegion& GetDestinationCopyRegion() const;
		std::span<const DataType> GetUpdateSourceDataSpan() const;

	private:
		D3D12::BufferCopyRegion mDestCopyRegion;
		std::vector<DataType> mDataToCopyArr;
	};
}

// ------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <GPUSceneBufferID BufferID>
	GPUSceneBufferUpdateOperation<BufferID>::GPUSceneBufferUpdateOperation(const D3D12::BufferCopyRegion& destCopyRegion) :
		mDestCopyRegion(destCopyRegion),
		mDataToCopyArr()
	{
		mDataToCopyArr.resize(destCopyRegion.GetCopyRegionSize() / sizeof(DataType));
	}

	template <GPUSceneBufferID BufferID>
	void GPUSceneBufferUpdateOperation<BufferID>::SetUpdateSourceData(const std::span<const DataType> dataSpan)
	{
		assert(dataSpan.size() == mDataToCopyArr.size() && "ERROR: An attempt was made to provide an invalid number of elements to a GPUSceneBufferUpdateOperation as determined by its assigned BufferCopyRegion!");
		std::ranges::copy(dataSpan, mDataToCopyArr.begin());
	}

	template <GPUSceneBufferID BufferID>
	const D3D12::BufferCopyRegion& GPUSceneBufferUpdateOperation<BufferID>::GetDestinationCopyRegion() const
	{
		return mDestCopyRegion;
	}

	template <GPUSceneBufferID BufferID>
	std::span<const typename GPUSceneBufferUpdateOperation<BufferID>::DataType> GPUSceneBufferUpdateOperation<BufferID>::GetUpdateSourceDataSpan() const
	{
		return std::span<const DataType>{ mDataToCopyArr };
	}
}