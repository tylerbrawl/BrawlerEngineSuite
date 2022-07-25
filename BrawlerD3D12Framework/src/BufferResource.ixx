module;
#include <optional>
#include <cassert>
#include <vector>
#include <mutex>
#include <span>
#include "DxDef.h"

export module Brawler.D3D12.BufferResource;
import Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.TLSFAllocator;
import Util.General;
import Brawler.OptionalRef;
import Brawler.D3D12.TLSFAllocationRequestInfo;
import Brawler.D3D12.BufferSubAllocationReservation;
import Brawler.ThreadSafeVector;
import Brawler.D3D12.ShaderResourceView;

export namespace Brawler
{
	namespace D3D12
	{
		struct BufferResourceInitializationInfo;
		class I_BufferSubAllocation;
	}
}

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

// -----------------------------------------------------------------------------------------------------------------------------

export namespace Brawler
{
	namespace D3D12
	{
		class BufferResource final : public I_GPUResource
		{
		public:
			explicit BufferResource(const BufferResourceInitializationInfo& initInfo);

			BufferResource(const BufferResource& rhs) = delete;
			BufferResource& operator=(const BufferResource& rhs) = delete;

			BufferResource(BufferResource&& rhs) noexcept = default;
			BufferResource& operator=(BufferResource&& rhs) noexcept = default;

			/// <summary>
			/// Creates a sub-allocation from this BufferResource for use as a sub-allocation of
			/// type SubAllocationType. The values specified by args... are passed to the constructor
			/// of SubAllocationType.
			/// 
			/// It can be inefficient to create a BufferResource every time a buffer needs to hold
			/// a specific type of data (e.g., one buffer for a constant buffer, one for a
			/// ray-tracing acceleration structure, etc.). Instead, the Brawler Engine uses the idea
			/// of sub-allocations to represent segments of buffer memory dedicated to a particular
			/// use of a given BufferResource. That way, the same BufferResource instance can hold many 
			/// different types of data.
			/// 
			/// Of course, the (current) D3D12 API disallows putting segments of the same BufferResource
			/// into different resource states. Thus, even if you create multiple sub-allocations from
			/// a single resource, you may not be able to use them concurrently on the GPU timeline
			/// without specifying all of the required resource states.
			/// 
			/// Sub-allocations within a BufferResource are represented as derived classes of
			/// I_BufferSubAllocation, and each class represents a different possible use of that buffer
			/// data. Each derived I_BufferSubAllocation instance is assigned a BufferSubAllocationReservation
			/// if the sub-allocation was successful when calling this function. This reservation object
			/// represents a segment of memory.
			/// 
			/// There are two ways in which a sub-allocation's memory reservation can be re-purposed:
			///		- The actual sub-allocation object goes out of scope while still holding a reservation.
			///       In that case, the reservation is automatically returned to the BufferResource so that
			///       a new sub-allocation can be made.
			/// 
			///    - I_BufferSubAllocation::RevokeReservation() is called manually. This will return the
			///      reservation object so that it can either manually be assigned to a different
			///      sub-allocation object or, if the reservation object goes out of scope, automatically
			///      returned to the BufferResource object whose memory it partially represents. Note that
			///      when manually re-assigning reservations, one must make sure that the destination
			///      sub-allocation is compatible with the reservation by calling
			///      I_BufferSubAllocation::IsReservationCompatible(). This ensures that the reservation
			///      both has enough storage and is aligned properly for the sub-allocation.
			/// 
			/// It is not always necessary to delete a sub-allocation. For instance, a transient 
			/// BufferResource is typically created only for a specific task; after that, the Brawler Engine 
			/// will likely re-use its memory for another resource which can alias with it.
			/// 
			/// *WARNING*: Sub-allocations from a BufferResource may fail, either because the buffer does
			/// not have enough free space or because of fragmentation. (Alignment requirements of the
			/// sub-allocation may also play a role in this failure.) *ALWAYS* check the returned
			/// std::optional instance!
			/// </summary>
			/// <typeparam name="SubAllocationType">
			/// - The type of sub-allocation which is to be created from this BufferResource instance.
			///   Different sub-allocation types are available for different uses of a buffer's memory.
			///   For instance, ConstantBufferSubAllocation allows one to dedicate a segment of a
			///   buffer for constant buffer data.
			/// </typeparam>
			/// <typeparam name="...Args">
			/// - The types of the arguments specified by args....
			/// </typeparam>
			/// <param name="...args">
			/// - The arguments which are to be passed into the constructor of SubAllocationType.
			/// </param>
			/// <returns>
			/// If the required segment of memory from the BufferResource could be allocated, then the
			/// returned std::optional instance has a value, and this value is a SubAllocationType which 
			/// refers to the created sub-allocation. This sub-allocation object is assigned a reservation.
			/// 
			/// Otherwise, the returned std::optional instance has no value.
			/// 
			/// *WARNING*: Sub-allocations from a BufferResource may fail, either because the buffer does
			/// not have enough free space or because of fragmentation. (Alignment requirements of the
			/// sub-allocation may also play a role in this failure.) *ALWAYS* check the returned
			/// std::optional instance!
			/// </returns>
			template <typename SubAllocationType, typename... Args>
				requires std::derived_from<SubAllocationType, I_BufferSubAllocation>
			[[nodiscard]] std::optional<SubAllocationType> CreateBufferSubAllocation(Args&&... args);

			[[nodiscard]] bool AssignReservation(I_BufferSubAllocation& subAllocation);

			template <DXGI_FORMAT Format>
			BindlessSRVAllocation CreateBindlessSRV(const ShaderResourceView<Format, D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_BUFFER>& srv);

		protected:
			void ExecutePostD3D12ResourceInitializationCallback() override;

		public:
			bool CanAliasBeforeUseOnGPU() const override;
			bool CanAliasAfterUseOnGPU() const override;

		private:
			BufferSubAllocationManager mSubAllocationManager;
		};
	}
}

// -----------------------------------------------------------------------------------------------------------------------------

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

// -----------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <typename SubAllocationType, typename... Args>
			requires std::derived_from<SubAllocationType, I_BufferSubAllocation>
		[[nodiscard]] std::optional<SubAllocationType> BufferResource::CreateBufferSubAllocation(Args&&... args)
		{
			return std::optional<SubAllocationType>{ mSubAllocationManager.CreateBufferSubAllocation<SubAllocationType, Args...>(std::forward<Args>(args)...) };
		}

		template <DXGI_FORMAT Format>
		BindlessSRVAllocation BufferResource::CreateBindlessSRV(const ShaderResourceView<Format, D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_BUFFER>& srv)
		{
			assert(&(srv.GetGPUResource()) == this && "ERROR: BufferResource::CreateBindlessSRV() was provided an SRV referring to a different BufferResource instance!");
			return I_GPUResource::CreateBindlessSRV(srv.CreateSRVDescription());
		}
	}
}