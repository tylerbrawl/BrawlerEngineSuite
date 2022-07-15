module;
#include <span>
#include <limits>
#include <cassert>

export module Brawler.D3D12.ByteAddressBufferSubAllocation;
import Brawler.D3D12.I_BufferSubAllocation;
import Brawler.D3D12.I_BufferSnapshot;
import Brawler.D3D12.BufferCopyRegion;

namespace Brawler
{
	namespace D3D12
	{
		template <std::size_t NumBytes>
		class ByteCountContainer
		{
		protected:
			constexpr ByteCountContainer() = default;

			static consteval std::size_t GetBytesCount()
			{
				return NumBytes;
			}
		};

		static constexpr std::size_t DYNAMIC_EXTENT = std::numeric_limits<std::size_t>::max();

		template <>
		class ByteCountContainer<DYNAMIC_EXTENT>
		{
		protected:
			constexpr ByteCountContainer(const std::size_t numBytes) :
				mByteCount(numBytes)
			{}

			constexpr std::size_t GetBytesCount() const
			{
				return mByteCount;
			}

		private:
			std::size_t mByteCount;
		};
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		template <std::size_t NumBytes = DYNAMIC_EXTENT>
		class ByteAddressBufferSubAllocation final : public I_BufferSubAllocation, private ByteCountContainer<NumBytes>
		{
		public:
			ByteAddressBufferSubAllocation() requires (NumBytes != DYNAMIC_EXTENT) = default;
			explicit ByteAddressBufferSubAllocation(const std::size_t numBytes) requires (NumBytes == DYNAMIC_EXTENT);

			ByteAddressBufferSubAllocation(const ByteAddressBufferSubAllocation& rhs) = delete;
			ByteAddressBufferSubAllocation& operator=(const ByteAddressBufferSubAllocation& rhs) = delete;

			ByteAddressBufferSubAllocation(ByteAddressBufferSubAllocation&& rhs) noexcept = default;
			ByteAddressBufferSubAllocation& operator=(ByteAddressBufferSubAllocation&& rhs) noexcept = default;

			std::size_t GetSubAllocationSize() const override;
			std::size_t GetRequiredDataPlacementAlignment() const override;

			void WriteRawBytesToBuffer(const std::span<const std::byte> srcDataSpan, const std::size_t offsetFromSubAllocationStart = 0) const;
			void ReadRawBytesFromBuffer(const std::span<std::byte> destDataSpan, const std::size_t offsetFromSubAllocationStart = 0) const;

			BufferCopyRegion GetBufferCopyRegion(const std::size_t offsetFromSubAllocationStart = 0, const std::size_t regionSizeInBytes = GetSubAllocationSize()) const;
		};
	}
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

export namespace Brawler
{
	namespace D3D12
	{
		template <std::size_t NumBytes = DYNAMIC_EXTENT>
		class ByteAddressBufferSnapshot final : public I_BufferSnapshot
		{
		public:
			explicit ByteAddressBufferSnapshot(const ByteAddressBufferSubAllocation<NumBytes>& byteAddressSubAllocation);

			ByteAddressBufferSnapshot(const ByteAddressBufferSnapshot& rhs) = default;
			ByteAddressBufferSnapshot& operator=(const ByteAddressBufferSnapshot& rhs) = default;

			ByteAddressBufferSnapshot(ByteAddressBufferSnapshot&& rhs) noexcept = default;
			ByteAddressBufferSnapshot& operator=(ByteAddressBufferSnapshot&& rhs) noexcept = default;

			BufferCopyRegion GetBufferCopyRegion(const std::size_t offsetFromSubAllocationStart = 0, const std::size_t regionSizeInBytes = GetSubAllocationSize()) const;
		};
	}
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <std::size_t NumBytes>
		ByteAddressBufferSubAllocation<NumBytes>::ByteAddressBufferSubAllocation(const std::size_t numBytes) requires (NumBytes == DYNAMIC_EXTENT) :
			I_BufferSubAllocation(),
			ByteCountContainer<NumBytes>(numBytes)
		{}

		template <std::size_t NumBytes>
		std::size_t ByteAddressBufferSubAllocation<NumBytes>::GetSubAllocationSize() const
		{
			return ByteCountContainer<NumBytes>::GetBytesCount();
		}

		template <std::size_t NumBytes>
		std::size_t ByteAddressBufferSubAllocation<NumBytes>::GetRequiredDataPlacementAlignment() const
		{
			return alignof(std::byte);
		}

		template <std::size_t NumBytes>
		void ByteAddressBufferSubAllocation<NumBytes>::WriteRawBytesToBuffer(const std::span<const std::byte> srcDataSpan, const std::size_t offsetFromSubAllocationStart) const
		{
			WriteToBuffer(srcDataSpan, offsetFromSubAllocationStart);
		}

		template <std::size_t NumBytes>
		void ByteAddressBufferSubAllocation<NumBytes>::ReadRawBytesFromBuffer(const std::span<std::byte> destDataSpan, const std::size_t offsetFromSubAllocationStart) const
		{
			ReadFromBuffer(destDataSpan, offsetFromSubAllocationStart);
		}

		template <std::size_t NumBytes>
		BufferCopyRegion ByteAddressBufferSubAllocation<NumBytes>::GetBufferCopyRegion(const std::size_t offsetFromSubAllocationStart, const std::size_t regionSizeInBytes) const
		{
			assert(offsetFromSubAllocationStart + regionSizeInBytes <= GetSubAllocationSize() && "ERROR: An invalid range of bytes was specified in a call to ByteAddressBufferSubAllocation::GetBufferCopyRegion()!");
			
			return BufferCopyRegion{ BufferCopyRegionInfo{
				.BufferResourcePtr = &(GetBufferResource()),
				.OffsetFromBufferStart = (GetOffsetFromBufferStart() + offsetFromSubAllocationStart),
				.RegionSizeInBytes = regionSizeInBytes
			} };
		}
	}
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <std::size_t NumBytes>
		ByteAddressBufferSnapshot<NumBytes>::ByteAddressBufferSnapshot(const ByteAddressBufferSubAllocation<NumBytes>& byteAddressSubAllocation) :
			I_BufferSnapshot(byteAddressSubAllocation)
		{}

		template <std::size_t NumBytes>
		BufferCopyRegion ByteAddressBufferSnapshot<NumBytes>::GetBufferCopyRegion(const std::size_t offsetFromSubAllocationStart, const std::size_t regionSizeInBytes) const
		{
			assert(offsetFromSubAllocationStart + regionSizeInBytes <= GetSubAllocationSize() && "ERROR: An invalid range of bytes was specified in a call to ByteAddressBufferSnapshot::GetBufferCopyRegion()!");

			return BufferCopyRegion{ BufferCopyRegionInfo{
				.BufferResourcePtr = &(GetBufferResource()),
				.OffsetFromBufferStart = (GetOffsetFromBufferStart() + offsetFromSubAllocationStart),
				.RegionSizeInBytes = regionSizeInBytes
			} };
		}
	}
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

export namespace Brawler
{
	namespace D3D12
	{
		using DynamicByteAddressBufferSubAllocation = ByteAddressBufferSubAllocation<DYNAMIC_EXTENT>;
		using DynamicByteAddressBufferSnapshot = ByteAddressBufferSnapshot<DYNAMIC_EXTENT>;
	}
}