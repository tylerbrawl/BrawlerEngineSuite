module;
#include <cstddef>
#include <cassert>
#include <span>
#include "DxDef.h"

export module Brawler.D3D12.BufferResourceDataMapping;
import Brawler.D3D12.I_BufferSubAllocation;
import Brawler.D3D12.BufferResource;
import Util.General;

namespace Brawler
{
	namespace D3D12
	{
		enum class DataMappingType
		{
			READ_ONLY,
			WRITE_ONLY
		};

		template <DataMappingType MappingType>
		struct DataMappingInfo
		{
			static_assert(sizeof(MappingType) != sizeof(MappingType));
		};

		template <>
		struct DataMappingInfo<DataMappingType::READ_ONLY>
		{
			using DataType = const std::byte;
		};

		template <>
		struct DataMappingInfo<DataMappingType::WRITE_ONLY>
		{
			using DataType = std::byte;
		};
	}
}

namespace Brawler
{
	namespace D3D12
	{
		template <Util::General::BuildMode BuildMode>
		class SubAllocationOffsetStorage
		{
		public:
			explicit SubAllocationOffsetStorage(const std::size_t subAllocationOffset)
			{}

			static consteval std::size_t GetStoredOffset()
			{
				return 0;
			}
		};

		template <>
		class SubAllocationOffsetStorage<Util::General::BuildMode::DEBUG>
		{
		public:
			explicit SubAllocationOffsetStorage(const std::size_t subAllocationOffset) :
				mSubAllocationOffset(subAllocationOffset)
			{}

			std::size_t GetStoredOffset() const
			{
				return mSubAllocationOffset;
			}

		private:
			std::size_t mSubAllocationOffset;
		};

		using CurrentSubAllocationOffsetStorage = SubAllocationOffsetStorage<Util::General::GetBuildMode()>;
	}
}

namespace Brawler
{
	namespace D3D12
	{
		template <DataMappingType MappingType>
		class BufferResourceDataMapping : private CurrentSubAllocationOffsetStorage
		{
		private:
			using DataType = typename DataMappingInfo<MappingType>::DataType;

		protected:
			BufferResourceDataMapping() = default;
			explicit BufferResourceDataMapping(const I_BufferSubAllocation& bufferSubAllocation);

		public:
			virtual ~BufferResourceDataMapping();

			BufferResourceDataMapping(const BufferResourceDataMapping& rhs) = delete;
			BufferResourceDataMapping& operator=(const BufferResourceDataMapping& rhs) = delete;

			BufferResourceDataMapping(BufferResourceDataMapping&& rhs) noexcept;
			BufferResourceDataMapping& operator=(BufferResourceDataMapping&& rhs) noexcept;

			std::size_t GetMappedDataSize() const;

		protected:
			std::span<DataType> GetMappedData() const;

		private:
			void UnMapData();

		private:
			std::span<DataType> mDataSpan;
			Brawler::D3D12Resource* mD3DResourcePtr;
		};
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <DataMappingType MappingType>
		BufferResourceDataMapping<MappingType>::BufferResourceDataMapping(const I_BufferSubAllocation& bufferSubAllocation) :
			CurrentSubAllocationOffsetStorage(bufferSubAllocation.GetOffsetFromBufferStart()),
			mDataSpan(),
			mD3DResourcePtr(&(bufferSubAllocation.GetD3D12Resource()))
		{
			using DataPointerType = std::add_pointer_t<DataType>;
			using ModifiableDataPointerType = std::add_pointer_t<std::remove_const_t<DataType>>;

			const std::size_t subAllocationOffset = bufferSubAllocation.GetOffsetFromBufferStart();
			const std::size_t subAllocationSize = bufferSubAllocation.GetSubAllocationSize();
			DataPointerType dataPointer = nullptr;

			if constexpr (MappingType == DataMappingType::READ_ONLY)
			{
				assert(bufferSubAllocation.GetBufferResource().GetHeapType() == D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK && "ERROR: An attempt was made to create a read-only data mapping to a BufferResource which was not located in a READBACK heap!");
				
				const D3D12_RANGE readRange{
					.Begin = subAllocationOffset,
					.End = (subAllocationOffset + subAllocationSize)
				};
				
				Util::General::CheckHRESULT(mD3DResourcePtr->Map(0, &readRange, reinterpret_cast<void**>(const_cast<ModifiableDataPointerType*>(&dataPointer))));
			}
			else
			{
				assert(bufferSubAllocation.GetBufferResource().GetHeapType() == D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD && "ERROR: An attempt was made to create a write-only data mapping to a BufferResource which was not located in an UPLOAD heap!");
				
				static constexpr D3D12_RANGE DISABLE_READ_RANGE{
					.Begin = 0,
					.End = 0
				};

				Util::General::CheckHRESULT(mD3DResourcePtr->Map(0, &DISABLE_READ_RANGE, reinterpret_cast<void**>(&dataPointer)));
			}

			// Offset the returned pointer manually based on where the sub-allocation starts.
			dataPointer += subAllocationOffset;

			mDataSpan = std::span<DataType>{ dataPointer, subAllocationSize };
		}

		template <DataMappingType MappingType>
		BufferResourceDataMapping<MappingType>::~BufferResourceDataMapping()
		{
			UnMapData();
		}

		template <DataMappingType MappingType>
		BufferResourceDataMapping<MappingType>::BufferResourceDataMapping(BufferResourceDataMapping&& rhs) noexcept :
			mDataSpan(rhs.mDataSpan),
			mD3DResourcePtr(rhs.mD3DResourcePtr)
		{
			rhs.mDataSpan = std::span<DataType>{};
			rhs.mD3DResourcePtr = nullptr;
		}

		template <DataMappingType MappingType>
		BufferResourceDataMapping<MappingType>& BufferResourceDataMapping<MappingType>::operator=(BufferResourceDataMapping&& rhs) noexcept
		{
			UnMapData();

			mDataSpan = rhs.mDataSpan;
			rhs.mDataSpan = std::span<DataType>{};

			mD3DResourcePtr = rhs.mD3DResourcePtr;
			rhs.mD3DResourcePtr = nullptr;

			return *this;
		}

		template <DataMappingType MappingType>
		std::size_t BufferResourceDataMapping<MappingType>::GetMappedDataSize() const
		{
			return mDataSpan.size_bytes();
		}

		template <DataMappingType MappingType>
		std::span<typename DataMappingInfo<MappingType>::DataType> BufferResourceDataMapping<MappingType>::GetMappedData() const
		{
			return mDataSpan;
		}

		template <DataMappingType MappingType>
		void BufferResourceDataMapping<MappingType>::UnMapData()
		{
			if (mD3DResourcePtr != nullptr) [[likely]]
			{
				// Specifying a value for pWrittenRange is only necessary for tooling, so we do so only in Debug
				// builds.
				if constexpr (Util::General::IsDebugModeEnabled())
				{
					if constexpr (MappingType == DataMappingType::READ_ONLY)
					{
						static constexpr D3D12_RANGE DISABLE_WRITE_RANGE{
							.Begin = 0,
							.End = 0
						};

						mD3DResourcePtr->Unmap(0, &DISABLE_WRITE_RANGE);
					}
					else
					{
						const D3D12_RANGE writeRange{
							.Begin = CurrentSubAllocationOffsetStorage::GetStoredOffset(),
							.End = CurrentSubAllocationOffsetStorage::GetStoredOffset() + mDataSpan.size_bytes()
						};

						mD3DResourcePtr->Unmap(0, &writeRange);
					}
				}
				else
					mD3DResourcePtr->Unmap(0, nullptr);

				mD3DResourcePtr = nullptr;
				mDataSpan = std::span<DataType>{};
			}
		}
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class UploadBufferResourceDataMapping final : public BufferResourceDataMapping<DataMappingType::WRITE_ONLY>
		{
		public:
			UploadBufferResourceDataMapping() = default;
			explicit UploadBufferResourceDataMapping(const I_BufferSubAllocation& bufferSubAllocation);

			UploadBufferResourceDataMapping(const UploadBufferResourceDataMapping& rhs) = delete;
			UploadBufferResourceDataMapping& operator=(const UploadBufferResourceDataMapping& rhs) = delete;

			UploadBufferResourceDataMapping(UploadBufferResourceDataMapping&& rhs) noexcept = default;
			UploadBufferResourceDataMapping& operator=(UploadBufferResourceDataMapping&& rhs) noexcept = default;

			// UPLOAD heaps reside in write-combine memory, which is incredibly slow to read from. Thus,
			// the API for UploadBufferResourceDataMapping does not expose a means to get access to the
			// underlying std::span. This prevents accidental reads.
			
			void WriteDataToBuffer(const std::span<const std::byte> srcDataSpan, const std::size_t offsetFromSubAllocationStart = 0) const;
		};

		class ReadBackBufferResourceDataMapping final : public BufferResourceDataMapping<DataMappingType::READ_ONLY>
		{
		public:
			ReadBackBufferResourceDataMapping() = default;
			explicit ReadBackBufferResourceDataMapping(const I_BufferSubAllocation& bufferSubAllocation);

			ReadBackBufferResourceDataMapping(const ReadBackBufferResourceDataMapping& rhs) = delete;
			ReadBackBufferResourceDataMapping& operator=(const ReadBackBufferResourceDataMapping& rhs) = delete;

			ReadBackBufferResourceDataMapping(ReadBackBufferResourceDataMapping&& rhs) noexcept = default;
			ReadBackBufferResourceDataMapping& operator=(ReadBackBufferResourceDataMapping&& rhs) noexcept = default;

			std::span<const std::byte> GetReadBackDataSpan() const;
		};
	}
}

// Add equivalent type aliases for those who prefer to think in terms of CPU page properties.

export namespace Brawler
{
	namespace D3D12
	{
		using WriteCombineBufferResourceDataMapping = UploadBufferResourceDataMapping;
		using WriteBackBufferResourceDataMapping = ReadBackBufferResourceDataMapping;
	}
}