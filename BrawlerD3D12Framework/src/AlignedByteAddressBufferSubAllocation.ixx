module;
#include <limits>
#include <span>
#include <cassert>

export module Brawler.D3D12.AlignedByteAddressBufferSubAllocation;
import Brawler.D3D12.I_BufferSubAllocation;
import Brawler.D3D12.I_BufferSnapshot;
import Brawler.D3D12.BufferCopyRegion;

namespace Brawler
{
	namespace D3D12
	{
		static constexpr std::size_t DYNAMIC_EXTENT = std::numeric_limits<std::size_t>::max();

		template <std::size_t SizeInBytes>
		class SizeContainer
		{
		public:
			constexpr SizeContainer() = default;

			constexpr virtual ~SizeContainer() = default;

			constexpr SizeContainer(const SizeContainer& rhs) = default;
			constexpr SizeContainer& operator=(const SizeContainer& rhs) = default;

			constexpr SizeContainer(SizeContainer&& rhs) noexcept = default;
			constexpr SizeContainer& operator=(SizeContainer&& rhs) noexcept = default;

			static consteval std::size_t GetSize()
			{
				return SizeInBytes;
			}
		};

		template <>
		class SizeContainer<DYNAMIC_EXTENT>
		{
		public:
			SizeContainer() = default;

			explicit SizeContainer(const std::size_t sizeInBytes) :
				mSizeInBytes(sizeInBytes)
			{}

			virtual ~SizeContainer() = default;

			SizeContainer(const SizeContainer& rhs) = default;
			SizeContainer& operator=(const SizeContainer& rhs) = default;

			SizeContainer(SizeContainer&& rhs) noexcept = default;
			SizeContainer& operator=(SizeContainer&& rhs) noexcept = default;

			std::size_t GetSize() const
			{
				return mSizeInBytes;
			}

		private:
			std::size_t mSizeInBytes;
		};
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		template <std::size_t SizeInBytes, std::size_t AlignmentInBytes>
		class AlignedByteAddressBufferSubAllocation final : public I_BufferSubAllocation, private SizeContainer<SizeInBytes>
		{
		public:
			AlignedByteAddressBufferSubAllocation() = default;

			explicit AlignedByteAddressBufferSubAllocation(const std::size_t sizeInBytes) requires (SizeInBytes == DYNAMIC_EXTENT) :
				I_BufferSubAllocation(),
				SizeContainer<SizeInBytes>(sizeInBytes)
			{}

			AlignedByteAddressBufferSubAllocation(const AlignedByteAddressBufferSubAllocation& rhs) = delete;
			AlignedByteAddressBufferSubAllocation& operator=(const AlignedByteAddressBufferSubAllocation& rhs) = delete;

			AlignedByteAddressBufferSubAllocation(AlignedByteAddressBufferSubAllocation&& rhs) noexcept = default;
			AlignedByteAddressBufferSubAllocation& operator=(AlignedByteAddressBufferSubAllocation&& rhs) noexcept = default;

			std::size_t GetSubAllocationSize() const override;
			std::size_t GetRequiredDataPlacementAlignment() const override;

			void WriteRawBytesToBuffer(const std::span<const std::byte> srcDataSpan, const std::size_t offsetFromSubAllocationStart = 0) const;
			void ReadRawBytesFromBuffer(const std::span<std::byte> destDataSpan, const std::size_t offsetFromSubAllocationStart = 0) const;

			BufferCopyRegion GetBufferCopyRegion(const std::size_t offsetFromSubAllocationStart = 0, const std::size_t regionSizeInBytes = GetSubAllocationSize()) const;
		};
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

export namespace Brawler
{
	namespace D3D12
	{
		template <std::size_t SizeInBytes, std::size_t AlignmentInBytes>
		class AlignedByteAddressBufferSnapshot final : public I_BufferSnapshot
		{
		public:
			explicit AlignedByteAddressBufferSnapshot(const AlignedByteAddressBufferSubAllocation<SizeInBytes, AlignmentInBytes>& subAllocation);

			AlignedByteAddressBufferSnapshot(const AlignedByteAddressBufferSnapshot& rhs) = default;
			AlignedByteAddressBufferSnapshot& operator=(const AlignedByteAddressBufferSnapshot& rhs) = default;

			AlignedByteAddressBufferSnapshot(AlignedByteAddressBufferSnapshot&& rhs) noexcept = default;
			AlignedByteAddressBufferSnapshot& operator=(AlignedByteAddressBufferSnapshot&& rhs) noexcept = default;

			BufferCopyRegion GetBufferCopyRegion(const std::size_t offsetFromSubAllocationStart = 0, const std::size_t regionSizeInBytes = GetSubAllocationSize()) const;
		};
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <std::size_t SizeInBytes, std::size_t AlignmentInBytes>
		std::size_t AlignedByteAddressBufferSubAllocation<SizeInBytes, AlignmentInBytes>::GetSubAllocationSize() const
		{
			return SizeContainer<SizeInBytes>::GetSize();
		}

		template <std::size_t SizeInBytes, std::size_t AlignmentInBytes>
		std::size_t AlignedByteAddressBufferSubAllocation<SizeInBytes, AlignmentInBytes>::GetRequiredDataPlacementAlignment() const
		{
			return AlignmentInBytes;
		}

		template <std::size_t SizeInBytes, std::size_t AlignmentInBytes>
		void AlignedByteAddressBufferSubAllocation<SizeInBytes, AlignmentInBytes>::WriteRawBytesToBuffer(const std::span<const std::byte> srcDataSpan, const std::size_t offsetFromSubAllocationStart) const
		{
			WriteToBuffer(srcDataSpan, offsetFromSubAllocationStart);
		}

		template <std::size_t SizeInBytes, std::size_t AlignmentInBytes>
		void AlignedByteAddressBufferSubAllocation<SizeInBytes, AlignmentInBytes>::ReadRawBytesFromBuffer(const std::span<std::byte> destDataSpan, const std::size_t offsetFromSubAllocationStart) const
		{
			ReadFromBuffer(destDataSpan, offsetFromSubAllocationStart);
		}

		template <std::size_t SizeInBytes, std::size_t AlignmentInBytes>
		BufferCopyRegion AlignedByteAddressBufferSubAllocation<SizeInBytes, AlignmentInBytes>::GetBufferCopyRegion(const std::size_t offsetFromSubAllocationStart, const std::size_t regionSizeInBytes) const
		{
			assert((offsetFromSubAllocationStart + regionSizeInBytes) <= GetSubAllocationSize());

			return BufferCopyRegion{ BufferCopyRegionInfo{
				.BufferResourcePtr = &(GetBufferResource()),
				.OffsetFromBufferStart = (GetOffsetFromBufferStart() + offsetFromSubAllocationStart),
				.RegionSizeInBytes = regionSizeInBytes
			} };
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <std::size_t SizeInBytes, std::size_t AlignmentInBytes>
		AlignedByteAddressBufferSnapshot<SizeInBytes, AlignmentInBytes>::AlignedByteAddressBufferSnapshot(const AlignedByteAddressBufferSubAllocation<SizeInBytes, AlignmentInBytes>& subAllocation) :
			I_BufferSnapshot(subAllocation)
		{}

		template <std::size_t SizeInBytes, std::size_t AlignmentInBytes>
		BufferCopyRegion AlignedByteAddressBufferSnapshot<SizeInBytes, AlignmentInBytes>::GetBufferCopyRegion(const std::size_t offsetFromSubAllocationStart, const std::size_t regionSizeInBytes) const
		{
			assert((offsetFromSubAllocationStart + regionSizeInBytes) <= GetSubAllocationSize());

			return BufferCopyRegion{ BufferCopyRegionInfo{
				.BufferResourcePtr = &(GetBufferResource()),
				.OffsetFromBufferStart = (GetOffsetFromBufferStart() + offsetFromSubAllocationStart),
				.RegionSizeInBytes = regionSizeInBytes
			} };
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

export namespace Brawler
{
	namespace D3D12
	{
		template <std::size_t AlignmentInBytes>
		using DynamicAlignedByteAddressBufferSubAllocation = AlignedByteAddressBufferSubAllocation<DYNAMIC_EXTENT, AlignmentInBytes>;

		template <std::size_t AlignmentInBytes>
		using DynamicAlignedByteAddressBufferSnapshot = AlignedByteAddressBufferSnapshot<DYNAMIC_EXTENT, AlignmentInBytes>;
	}
}