module;
#include <memory>
#include <optional>
#include <mutex>
#include <span>
#include <cassert>
#include "DxDef.h"

module Brawler.D3D12.BufferResource;
import Brawler.D3D12.I_BufferSubAllocation;
import Util.General;
import Brawler.D3D12.BufferSubAllocationReservationHandle;

namespace
{
	static constexpr D3D12_RANGE DISABLE_READ_RANGE{
		.Begin = 0,
		.End = 0
	};

	static constexpr D3D12_RANGE DISABLE_WRITE_RANGE{
		.Begin = 0,
		.End = 0
	};
}

namespace Brawler
{
	namespace D3D12
	{
		BufferSubAllocationManager::BufferSubAllocationManager(BufferResource& owningBufferResource, const std::size_t sizeInBytes) :
			mBufferMemoryAllocator(),
			mOwningBufferResourcePtr(&owningBufferResource),
			mPendingWriteRequestArr(),
			mReservationPtrArr(),
			mCritSection()
		{
			mBufferMemoryAllocator.Initialize(sizeInBytes);
		}

		Brawler::D3D12Resource& BufferSubAllocationManager::GetBufferD3D12Resource() const
		{
			assert(mOwningBufferResourcePtr != nullptr);
			return mOwningBufferResourcePtr->GetD3D12Resource();
		}

		BufferResource& BufferSubAllocationManager::GetBufferResource()
		{
			assert(mOwningBufferResourcePtr != nullptr);
			return *mOwningBufferResourcePtr;
		}

		const BufferResource& BufferSubAllocationManager::GetBufferResource() const
		{
			assert(mOwningBufferResourcePtr != nullptr);
			return *mOwningBufferResourcePtr;
		}

		D3D12_GPU_VIRTUAL_ADDRESS BufferSubAllocationManager::GetBufferGPUVirtualAddress() const
		{
			assert(mOwningBufferResourcePtr != nullptr);
			return mOwningBufferResourcePtr->GetD3D12Resource().GetGPUVirtualAddress();
		}

		void BufferSubAllocationManager::DeleteSubAllocation(BufferSubAllocationReservation& reservation)
		{
			assert(&(reservation.GetBufferSubAllocationManager()) == this);

			mBufferMemoryAllocator.DeleteAllocation(reservation.GetTLSFMemoryBlock());
		}

		void BufferSubAllocationManager::WriteToBuffer(const std::span<const std::byte> srcDataByteSpan, const std::size_t bufferOffset)
		{
			{
				std::scoped_lock<std::mutex> lock{ mCritSection };

				// If the resource has not been created yet, then write the contents into
				// the temporary CPU storage.
				if (!mOwningBufferResourcePtr->IsD3D12ResourceCreated())
				{
					DataWriteRequest writeRequest{
						.DataArr{},
						.BufferOffset = bufferOffset
					};
					writeRequest.DataArr.resize(srcDataByteSpan.size_bytes());

					std::memcpy(writeRequest.DataArr.data(), srcDataByteSpan.data(), srcDataByteSpan.size_bytes());

					mPendingWriteRequestArr.push_back(std::move(writeRequest));
					return;
				}
			}
			
			static constexpr D3D12_RANGE DISABLE_READ_RANGE{
				.Begin = 0,
				.End = 0
			};

			Brawler::D3D12Resource& d3dBufferResource{ mOwningBufferResourcePtr->GetD3D12Resource() };

			std::uint8_t* destinationPtr = nullptr;
			Util::General::CheckHRESULT(d3dBufferResource.Map(0, &DISABLE_READ_RANGE, reinterpret_cast<void**>(&destinationPtr)));
			destinationPtr += bufferOffset;

			// Copy the data into the buffer. Rather than going in a for-loop for each resource
			// in the std::span, since we know that a std::span represents a contiguous region
			// of memory, we can just get the address of the first element in the span. This
			// should, in general, be much more efficient.
			std::memcpy(destinationPtr, srcDataByteSpan.data(), srcDataByteSpan.size_bytes());

			// Unmap destinationPtr. In Debug builds, we specify the region which we wrote to.
			// According to the MSDN, this can improve tool support, but otherwise has no impact
			// on the correctness of the application.
			if constexpr (Util::General::IsDebugModeEnabled())
			{
				const D3D12_RANGE writtenRange{
					.Begin = bufferOffset,
					.End = bufferOffset + srcDataByteSpan.size_bytes()
				};

				d3dBufferResource.Unmap(0, &writtenRange);
			}
			else
				d3dBufferResource.Unmap(0, nullptr);
		}

		void BufferSubAllocationManager::ReadFromBuffer(const std::span<std::byte> destDataByteSpan, const std::size_t bufferOffset)
		{
			// It makes no sense to read from the BufferResource before its ID3D12Resource has been
			// created. Strictly speaking, this assert cannot actually detect race conditions until
			// we get a bad data race, but having one check is better than having none.
			assert(mOwningBufferResourcePtr != nullptr && mOwningBufferResourcePtr->IsD3D12ResourceCreated() && "ERROR: An attempt was made to read from a BufferResource before its ID3D12Resource object could be created! (This is probably a *race condition!*)");

			const D3D12_RANGE readRange{
				.Begin = bufferOffset,
				.End = bufferOffset + destDataByteSpan.size_bytes()
			};

			Brawler::D3D12Resource& d3dBufferResource{ mOwningBufferResourcePtr->GetD3D12Resource() };

			std::uint8_t* srcPtr = nullptr;
			Util::General::CheckHRESULT(d3dBufferResource.Map(0, &readRange, reinterpret_cast<void**>(&srcPtr)));
			srcPtr += bufferOffset;

			// Copy the data from the buffer. Rather than going in a for-loop for each resource
			// in the std::span, since we know that a std::span represents a contiguous region
			// of memory, we can just get the address of the first element in the span. This
			// should, in general, be much more efficient.
			std::memcpy(destDataByteSpan.data(), srcPtr, destDataByteSpan.size_bytes());

			// Unmap srcPtr. In Debug builds, we specify the region which we wrote to.
			// According to the MSDN, this can improve tool support, but otherwise has no impact
			// on the correctness of the application.
			if constexpr (Util::General::IsDebugModeEnabled())
				d3dBufferResource.Unmap(0, &DISABLE_WRITE_RANGE);
			else
				d3dBufferResource.Unmap(0, nullptr);
		}

		void BufferSubAllocationManager::OnD3D12ResourceInitialized()
		{
			// For any sub-allocations which wrote into temporary CPU storage, transfer the
			// contents of that allocation to the GPU. We only do this for BufferResources
			// located in D3D12_HEAP_TYPE_UPLOAD heaps, though.

			if (mOwningBufferResourcePtr->GetHeapType() == D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD)
				TransferTemporaryCPUDataToGPUBuffer();
		}

		bool BufferSubAllocationManager::AssignReservationToSubAllocation(I_BufferSubAllocation& subAllocation)
		{
			// First, try to return any BufferSubAllocationReservation instances which are
			// ready for destruction.
			mReservationPtrArr.EraseIf([this] (const std::unique_ptr<BufferSubAllocationReservation>& reservationPtr)
			{
				if (reservationPtr->ReadyForDestruction()) [[unlikely]]
				{
					DeleteSubAllocation(*reservationPtr);
					return true;
				}

				return false;
			});
			
			const TLSFAllocationRequestInfo allocationRequest{
				.SizeInBytes = subAllocation.GetSubAllocationSize(),
				.Alignment = subAllocation.GetRequiredDataPlacementAlignment()
			};
			Brawler::OptionalRef<TLSFMemoryBlock> subAllocationMemoryBlock{ mBufferMemoryAllocator.CreateAllocation(allocationRequest) };

			if (!subAllocationMemoryBlock.HasValue()) [[unlikely]]
				return false;

			std::unique_ptr<BufferSubAllocationReservation> reservationPtr{ std::make_unique<BufferSubAllocationReservation>() };
			reservationPtr->SetOwningManager(*this);
			reservationPtr->SetTLSFMemoryBlock(*subAllocationMemoryBlock);

			subAllocation.AssignReservation(reservationPtr->CreateHandle());
			mReservationPtrArr.PushBack(std::move(reservationPtr));

			return true;
		}

		void BufferSubAllocationManager::TransferTemporaryCPUDataToGPUBuffer()
		{
			std::scoped_lock<std::mutex> lock{ mCritSection };

			if (mPendingWriteRequestArr.empty()) [[likely]]
				return;

			// We split the implementation of this into two paths, depending on whether or
			// not the program is build for Debug builds.
			//
			//   - In Debug builds, we call ID3D12Resource::Map() and ID3D12Resource::Unmap()
			//     for every pending DataWriteRequest. This improves tooling support by telling
			//     the API exactly which ranges we are writing to.
			//
			//   - In Release builds, we call ID3D12Resource::Map() and ID3D12Resource::Unmap()
			//     only once, and offset the returned CPU virtual address pointer as needed to
			//     perform the memory copies.

			Brawler::D3D12Resource& d3dBufferResource{ mOwningBufferResourcePtr->GetD3D12Resource() };

			if constexpr (Util::General::IsDebugModeEnabled())
			{
				for (const auto& dataWriteRequest : mPendingWriteRequestArr)
				{
					std::uint8_t* destinationPtr = nullptr;
					Util::General::CheckHRESULT(d3dBufferResource.Map(0, &DISABLE_READ_RANGE, reinterpret_cast<void**>(&destinationPtr)));
					destinationPtr += dataWriteRequest.BufferOffset;

					std::memcpy(destinationPtr, dataWriteRequest.DataArr.data(), dataWriteRequest.DataArr.size());

					const D3D12_RANGE writeRange{
						.Begin = dataWriteRequest.BufferOffset,
						.End = dataWriteRequest.BufferOffset + dataWriteRequest.DataArr.size()
					};
					d3dBufferResource.Unmap(0, &writeRange);
				}
			}
			else
			{
				std::uint8_t* baseBufferPtr = nullptr;
				Util::General::CheckHRESULT(d3dBufferResource.Map(0, &DISABLE_READ_RANGE, reinterpret_cast<void**>(&baseBufferPtr)));

				for (const auto& dataWriteRequest : mPendingWriteRequestArr)
					std::memcpy(baseBufferPtr + dataWriteRequest.BufferOffset, dataWriteRequest.DataArr.data(), dataWriteRequest.DataArr.size());

				d3dBufferResource.Unmap(0, nullptr);
			}

			mPendingWriteRequestArr.clear();
		}
	}
}