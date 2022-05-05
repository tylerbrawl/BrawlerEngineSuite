module;
#include <span>
#include <memory>
#include "DxDef.h"

export module Brawler.D3D12.I_BufferSubAllocation;
export import Brawler.D3D12.BufferSubAllocationReservation;

export namespace Brawler
{
	namespace D3D12
	{
		class BufferResource;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class I_BufferSubAllocation
		{
		protected:
			I_BufferSubAllocation() = default;

		public:
			virtual ~I_BufferSubAllocation() = default;

			I_BufferSubAllocation(const I_BufferSubAllocation& rhs) = delete;
			I_BufferSubAllocation& operator=(const I_BufferSubAllocation& rhs) = delete;

			I_BufferSubAllocation(I_BufferSubAllocation&& rhs) noexcept = default;
			I_BufferSubAllocation& operator=(I_BufferSubAllocation&& rhs) noexcept = default;

		protected:
			std::size_t GetOffsetFromBufferStart() const;

		public:
			Brawler::D3D12Resource& GetD3D12Resource() const;
			const BufferResource& GetBufferResource() const;

			/// <summary>
			/// Calculates and returns the GPU virtual address of this sub-allocation, and *NOT*
			/// the GPU virtual address of the buffer! (The latter value can be calculated by
			/// calculating (GetGPUVirtualAddress() - GetOffsetFromBufferStart()).)
			/// </summary>
			/// <returns>
			/// The function returns the GPU virtual address of this sub-allocation.
			/// </returns>
			D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const;

			virtual std::size_t GetSubAllocationSize() const = 0;
			virtual std::size_t GetRequiredDataPlacementAlignment() const = 0;

		public:
			bool IsReservationCompatible(const BufferSubAllocationReservation& reservation) const;

			void AssignReservation(std::unique_ptr<BufferSubAllocationReservation>&& reservation);
			std::unique_ptr<BufferSubAllocationReservation> RevokeReservation();

		protected:
			/// <summary>
			/// This function is called by the I_BufferSubAllocation instance whenever it is
			/// assigned a new BufferSubAllocationReservation instance. Derived classes can
			/// override this function to perform actions which need to occur after a new
			/// reservation is assigned.
			/// 
			/// For instance, TextureCopyBufferSubAllocation overrides this function to
			/// cache the new offset from the buffer start whenever a new reservation is
			/// assigned.
			/// </summary>
			virtual void OnReservationAssigned();

		public:
			/// <summary>
			/// Describes whether or not this I_BufferSubAllocation instance has an assigned
			/// SubBufferAllocationReservation. If this is the case (i.e., the function returns
			/// true), then the instance has been granted a segment of memory from within
			/// a BufferResource.
			/// 
			/// One should only use an I_BufferSubAllocation instance which has a reservation.
			/// Failing to ensure this will result in *undefined* behavior. Keep in mind, however,
			/// that just because a reservation exists, that does *NOT* mean that actual GPU
			/// memory has been allocated yet. All read operations require this memory to be
			/// allocated, but write operations can be executed early.
			/// 
			/// To check whether or not the backing GPU memory has been allocated, you must
			/// call I_GPUResource::IsD3D12ResourceCreated() on the I_GPUResource instance
			/// returned by calling I_BufferSubAllocation::GetBufferResource(). Keep in mind that
			/// waiting in a spinlock for the resource to be created may result in a deadlock.
			/// </summary>
			/// <returns>
			/// The function returns true if a BufferSubAllocationReservation was assigned to this
			/// I_BufferSubAllocation instance and false otherwise.
			/// </returns>
			bool HasReservation() const;

		protected:
			BufferSubAllocationManager& GetOwningManager() const;

		protected:
			/// <summary>
			/// Writes the data provided in srcDataSpan to the BufferResource. The data is written at an offset
			/// subAllocationOffsetInBytes away from the start of the *sub-allocation*, and NOT the start 
			/// of the buffer.
			/// 
			/// It is *NOT* required to wait for the backing GPU memory to be allocated before data can be
			/// written with this function. If I_BufferSubAllocation::WriteToBuffer() is called before the
			/// GPU memory is allocated, then the data which is to be written is copied into temporary CPU
			/// storage. It will then be written to the GPU immediately after the GPU memory has been
			/// allocated.
			/// 
			/// When possible or applicable, derived classes should use this function to create a more
			/// convenient API for modifying BufferResource data.
			/// 
			/// *NOTE*: This function will assert in Debug builds if it is called for a BufferResource which
			/// was not created in a D3D12_HEAP_TYPE_UPLOAD heap!
			/// </summary>
			/// <typeparam name="T">
			/// - The type of the data which is to be written.
			/// </typeparam>
			/// <param name="srcDataSpan">
			/// - A std::span containing a contiguous set of read-only data elements of type T. All of the 
			///   elements of this std::span will be written to the BufferResource.
			/// </param>
			/// <param name="subAllocationOffsetInBytes">
			/// - The offset, in bytes, from the start of the sub-allocation at which the data will be written
			///   into the BufferResource.
			/// </param>
			template <typename T>
			void WriteToBuffer(const std::span<const T> srcDataSpan, const std::size_t subAllocationOffsetInBytes) const;

			/// <summary>
			/// Reads data from the BufferResource and inserts it into the elements of destDataSpan. The data is
			/// read at an offset subAllocationOffsetInBytes away from the start of the *sub-allocation*, and NOT the
			/// start of the buffer. The amount of data read is equivalent to the combined sizes of all of the
			/// elements in destDataSpan.
			/// 
			/// Unlike I_BufferSubAllocation::WriteToBuffer(), it is *NOT* valid to call this function before
			/// the backing GPU memory for the sub-allocation has been created! (It doesn't make much sense to
			/// call this function before that point, anyways.)
			/// 
			/// When possible or applicable, derived classes should use this function to create a more convenient
			/// API for reading BufferResource data.
			/// 
			/// *NOTE*: This function will assert in Debug builds if it is called for a BufferResource which was
			/// not created in a D3D12_HEAP_TYPE_READBACK heap!
			/// </summary>
			/// <typeparam name="T">
			/// - The type of the data which is to be read.
			/// </typeparam>
			/// <param name="destDataSpan">
			/// - A std::span containing a contiguous set of writeable data elements of type T. All of the elements
			///   of this std::span will have their data be filled with that which is in the BufferResource. This
			///   implies that the size of the std::span directly determines how much data is read.
			/// </param>
			/// <param name="subAllocationOffsetInBytes">
			/// - The offset, in bytes, from the start of the sub-allocation at which the data will be read from
			///   the BufferResource.
			/// </param>
			template <typename T>
			void ReadFromBuffer(const std::span<T> destDataSpan, const std::size_t subAllocationOffsetInBytes) const;

		private:
			void WriteToBufferIMPL(const std::span<const std::byte> srcDataByteSpan, const std::size_t subAllocationOffsetInBytes) const;
			void ReadFromBufferIMPL(const std::span<std::byte> destDataByteSpan, const std::size_t subAllocationOffsetInBytes) const;

		private:
			std::unique_ptr<BufferSubAllocationReservation> mReservation;
		};
	}
}

// ----------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <typename T>
		void I_BufferSubAllocation::WriteToBuffer(const std::span<const T> srcDataSpan, const std::size_t subAllocationOffsetInBytes) const
		{
			WriteToBufferIMPL(std::as_bytes(srcDataSpan), subAllocationOffsetInBytes);
		}

		template <typename T>
		void I_BufferSubAllocation::ReadFromBuffer(const std::span<T> destDataSpan, const std::size_t subAllocationOffsetInBytes) const
		{
			ReadFromBufferIMPL(std::as_writable_bytes(destDataSpan), subAllocationOffsetInBytes);
		}
	}
}