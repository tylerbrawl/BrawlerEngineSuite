module;
#include <cassert>
#include <ranges>
#include <span>
#include <vector>

export module Brawler.GenericPreFrameBufferUpdate;
import Brawler.D3D12.BufferCopyRegion;
import Brawler.D3D12.BufferResource;

export namespace Brawler
{
	struct GenericPreFrameBufferUpdateInfo
	{
		D3D12::BufferResource* DestBufferResourcePtr;
		std::size_t OffsetFromBufferStart;
		std::size_t UpdateRegionSizeInBytes;
	};

	class GenericPreFrameBufferUpdate
	{
	public:
		GenericPreFrameBufferUpdate() = default;
		explicit GenericPreFrameBufferUpdate(GenericPreFrameBufferUpdateInfo&& updateInfo);

		GenericPreFrameBufferUpdate(const GenericPreFrameBufferUpdate& rhs) = delete;
		GenericPreFrameBufferUpdate& operator=(const GenericPreFrameBufferUpdate& rhs) = delete;

		GenericPreFrameBufferUpdate(GenericPreFrameBufferUpdate&& rhs) noexcept = default;
		GenericPreFrameBufferUpdate& operator=(GenericPreFrameBufferUpdate&& rhs) noexcept = default;

		template <typename T>
		void SetInputData(const std::span<const T> dataSpan);

		D3D12::BufferResource& GetBufferResource();
		D3D12::BufferCopyRegion GetBufferCopyRegion() const;

		std::span<const std::byte> GetDataByteSpan() const;
		std::vector<std::byte> ExtractDataByteArray();

		std::size_t GetUpdateRegionSize() const;

	private:
		GenericPreFrameBufferUpdateInfo mUpdateInfo;
		std::vector<std::byte> mDataByteArr;
	};
}

// -----------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename T>
	void GenericPreFrameBufferUpdate::SetInputData(const std::span<const T> dataSpan)
	{
		assert(dataSpan.size_bytes() == mUpdateInfo.UpdateRegionSizeInBytes);

		const std::span<const std::byte> srcDataByteSpan{ std::as_bytes(dataSpan) };
		std::ranges::copy(srcDataByteSpan, mDataByteArr.begin());
	}
}