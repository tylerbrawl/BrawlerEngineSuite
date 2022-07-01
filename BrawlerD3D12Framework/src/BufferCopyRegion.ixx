module;
#include <cstddef>
#include "DxDef.h"

export module Brawler.D3D12.BufferCopyRegion;
import Brawler.D3D12.BufferResource;

export namespace Brawler
{
	namespace D3D12
	{
		struct BufferCopyRegionInfo
		{
			const BufferResource* BufferResourcePtr;
			std::size_t OffsetFromBufferStart;
			std::size_t RegionSizeInBytes;
		};

		class BufferCopyRegion
		{
		public:
			BufferCopyRegion() = default;
			explicit BufferCopyRegion(BufferCopyRegionInfo&& regionInfo);

			BufferCopyRegion(const BufferCopyRegion& rhs) = default;
			BufferCopyRegion& operator=(const BufferCopyRegion& rhs) = default;

			BufferCopyRegion(BufferCopyRegion&& rhs) noexcept = default;
			BufferCopyRegion& operator=(BufferCopyRegion&& rhs) noexcept = default;

			const BufferResource& GetBufferResource() const;
			Brawler::D3D12Resource& GetD3D12Resource() const;

			std::size_t GetOffsetFromBufferStart() const;
			std::size_t GetCopyRegionSize() const;

		private:
			BufferCopyRegionInfo mRegionInfo;
		};
	}
}