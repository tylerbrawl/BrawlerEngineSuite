module;
#include <vector>
#include <cstdint>
#include <span>
#include <algorithm>
#include <cassert>

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

		template <typename U>
			requires std::is_same_v<std::decay_t<U>, std::decay_t<GPUSceneBufferElementType<BufferID>>>
		void SetUpdateSourceData(U&& data);

		const D3D12::BufferCopyRegion& GetDestinationCopyRegion() const;
		const DataType& GetUpdateSourceData() const;

	private:
		D3D12::BufferCopyRegion mDestCopyRegion;
		DataType mDataToCopy;
	};
}

// ------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <GPUSceneBufferID BufferID>
	GPUSceneBufferUpdateOperation<BufferID>::GPUSceneBufferUpdateOperation(const D3D12::BufferCopyRegion& destCopyRegion) :
		mDestCopyRegion(destCopyRegion),
		mDataToCopy()
	{}

	template <GPUSceneBufferID BufferID>
	template <typename U>
		requires std::is_same_v<std::decay_t<U>, std::decay_t<GPUSceneBufferElementType<BufferID>>>
	void GPUSceneBufferUpdateOperation<BufferID>::SetUpdateSourceData(U&& data)
	{
		mDataToCopy = std::forward<U>(data);
	}

	template <GPUSceneBufferID BufferID>
	const D3D12::BufferCopyRegion& GPUSceneBufferUpdateOperation<BufferID>::GetDestinationCopyRegion() const
	{
		return mDestCopyRegion;
	}

	template <GPUSceneBufferID BufferID>
	const typename GPUSceneBufferUpdateOperation<BufferID>::DataType& GPUSceneBufferUpdateOperation<BufferID>::GetUpdateSourceData() const
	{
		return mDataToCopy;
	}
}