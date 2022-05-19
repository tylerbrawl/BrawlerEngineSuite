module;
#include <memory>
#include <optional>
#include <mutex>
#include <span>
#include <cassert>
#include "DxDef.h"

export module Brawler.D3D12.BufferSubAllocationManager;
import Brawler.D3D12.TLSFAllocator;
import Util.General;
import Brawler.OptionalRef;
import Brawler.D3D12.TLSFAllocationRequestInfo;
import Brawler.D3D12.BufferSubAllocationReservation;
import Brawler.ThreadSafeVector;

export namespace Brawler
{
	namespace D3D12
	{
		class I_BufferSubAllocation;
		class BufferResource;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class BufferSubAllocationManager
		{
		private:
			struct DataWriteRequest
			{
				std::vector<std::byte> DataArr;
				std::size_t BufferOffset;
			};

		public:
			template <typename T>
			struct BufferAccessInfo
			{
				std::span<T> DataSpan;
				std::size_t BufferOffset;
			};

		private:
			friend class BufferResource;

		public:
			BufferSubAllocationManager(BufferResource& owningBufferResource, const std::size_t sizeInBytes);

			BufferSubAllocationManager(const BufferSubAllocationManager& rhs) = delete;
			BufferSubAllocationManager& operator=(const BufferSubAllocationManager& rhs) = delete;

			BufferSubAllocationManager(BufferSubAllocationManager&& rhs) noexcept = default;
			BufferSubAllocationManager& operator=(BufferSubAllocationManager&& rhs) noexcept = default;

			Brawler::D3D12Resource& GetBufferD3D12Resource() const;

			BufferResource& GetBufferResource();
			const BufferResource& GetBufferResource() const;

			D3D12_GPU_VIRTUAL_ADDRESS GetBufferGPUVirtualAddress() const;

			template <typename SubAllocationType, typename... Args>
			std::optional<SubAllocationType> CreateBufferSubAllocation(Args&&... args);

			void DeleteSubAllocation(BufferSubAllocationReservation& reservation);

			void WriteToBuffer(const std::span<const std::byte> srcDataByteSpan, const std::size_t bufferOffset);
			void ReadFromBuffer(const std::span<std::byte> destDataByteSpan, const std::size_t bufferOffset);

		private:
			void OnD3D12ResourceInitialized();
			bool AssignReservationToSubAllocation(I_BufferSubAllocation& subAllocation);

			void TransferTemporaryCPUDataToGPUBuffer();

		private:
			TLSFAllocator mBufferMemoryAllocator;
			BufferResource* mOwningBufferResourcePtr;
			std::vector<DataWriteRequest> mPendingWriteRequestArr;
			Brawler::ThreadSafeVector<std::unique_ptr<BufferSubAllocationReservation>> mReservationPtrArr;
			mutable std::mutex mCritSection;
		};
	}
}

// ---------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <typename SubAllocationType, typename... Args>
		std::optional<SubAllocationType> BufferSubAllocationManager::CreateBufferSubAllocation(Args&&... args)
		{
			SubAllocationType subAllocation{ std::forward<Args>(args)... };

			if (!AssignReservationToSubAllocation(subAllocation)) [[unlikely]]
				return std::optional<SubAllocationType>{};
			
			return std::optional<SubAllocationType>{ std::move(subAllocation) };
		}
	}
}