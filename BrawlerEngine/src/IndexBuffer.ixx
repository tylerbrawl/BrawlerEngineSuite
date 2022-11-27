module;
#include <cstddef>
#include <memory>
#include <span>
#include <DxDef.h>

export module Brawler.IndexBuffer;
import Brawler.D3D12.BufferResource;
import Brawler.D3D12.BindlessSRVAllocation;
import Brawler.D3D12.AlignedByteAddressBufferSubAllocation;
import Brawler.D3D12.GPUResourceViews;
import Brawler.GenericPreFrameBufferUpdate;
import Brawler.Application;
import Brawler.D3D12.Renderer;
import Brawler.GPUSceneUpdateRenderModule;

namespace Brawler
{
	namespace IMPL
	{
		template <std::integral IndexType>
		consteval DXGI_FORMAT GetIndexBufferFormat()
		{
			if constexpr (std::is_same_v<IndexType, std::uint16_t>)
				return DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
			else if constexpr (std::is_same_v<IndexType, std::uint32_t>)
				return DXGI_FORMAT::DXGI_FORMAT_R32_UINT;
			else
				return DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		}
	}
}

namespace Brawler 
{
	namespace IMPL
	{
		template <std::integral IndexType>
		class IndexBuffer
		{
		private:
			static constexpr DXGI_FORMAT INDEX_BUFFER_FORMAT = GetIndexBufferFormat<IndexType>();
			static_assert(INDEX_BUFFER_FORMAT != DXGI_FORMAT::DXGI_FORMAT_UNKNOWN, "ERROR: A DXGI_FORMAT value was never assigned to a specific Brawler::IMPL::IndexBuffer template instantiation! (See Brawler::IMPL::GetIndexBufferFormat() in IndexBuffer.ixx.)");

		private:
			using SubAllocationType = D3D12::DynamicAlignedByteAddressBufferSubAllocation<alignof(IndexType)>;

		public:
			IndexBuffer() = default;
			explicit IndexBuffer(const std::size_t numIndices);

			IndexBuffer(const IndexBuffer& rhs) = delete;
			IndexBuffer& operator=(const IndexBuffer& rhs) = delete;

			IndexBuffer(IndexBuffer&& rhs) noexcept = default;
			IndexBuffer& operator=(IndexBuffer&& rhs) noexcept = default;

			void SetIndices(const std::span<const IndexType> indexSpan);

			std::uint32_t GetBindlessSRVIndex() const;

		private:
			bool IsValidIndexBuffer() const;

		private:
			std::unique_ptr<D3D12::BufferResource> mIndexBufferPtr;
			SubAllocationType mIndexBufferSubAllocation;
			D3D12::BindlessSRVAllocation mBindlessSRVAllocation;
		};
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace IMPL
	{
		template <std::integral IndexType>
		IndexBuffer<IndexType>::IndexBuffer(const std::size_t numIndices) :
			mIndexBufferPtr(std::make_unique<D3D12::BufferResource>(D3D12::BufferResourceInitializationInfo{
				.SizeInBytes = (numIndices * sizeof(IndexType)),
				.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT
			})),
			mIndexBufferSubAllocation(numIndices),
			mBindlessSRVAllocation()
		{
			[[maybe_unused]] const bool wasReservationSuccessful = mIndexBufferPtr->AssignReservation(mIndexBufferSubAllocation);
			assert(wasReservationSuccessful);

			mBindlessSRVAllocation = mIndexBufferPtr->CreateBindlessSRV(D3D12::TypedBufferShaderResourceView<INDEX_BUFFER_FORMAT>{ *mIndexBufferPtr, D3D12_BUFFER_SRV{
				.FirstElement = 0,
				.NumElements = static_cast<std::uint32_t>(numIndices)
			} });
		}

		template <std::integral IndexType>
		void IndexBuffer<IndexType>::SetIndices(const std::span<const IndexType> indexSpan)
		{
			assert(IsValidIndexBuffer());
			assert(indexSpan.size_bytes() == mIndexBufferSubAllocation.GetSubAllocationSize());

			// Schedule an update for the next frame on the GPU to set the indices in the index
			// buffer to the appropriate values.
			GenericPreFrameBufferUpdate indexBufferUpdate{ GenericPreFrameBufferUpdateInfo{
				.DestBufferResourcePtr = mIndexBufferPtr.get(),
				.OffsetFromBufferStart = mIndexBufferSubAllocation.GetOffsetFromBufferStart(),
				.UpdateRegionSizeInBytes = mIndexBufferSubAllocation.GetSubAllocationSize()
			} };

			Brawler::GetRenderer().GetRenderModule<GPUSceneUpdateRenderModule>().ScheduleGenericBufferUpdateForNextFrame(std::move(indexBufferUpdate));
		}

		template <std::integral IndexType>
		std::uint32_t IndexBuffer<IndexType>::GetBindlessSRVIndex() const
		{
			assert(IsValidIndexBuffer());
			return mBindlessSRVAllocation.GetBindlessSRVIndex();
		}

		template <std::integral IndexType>
		bool IndexBuffer<IndexType>::IsValidIndexBuffer() const
		{
			return (mIndexBufferPtr != nullptr);
		}
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------------------

export namespace Brawler
{
	// We use 32-bit indices because our index buffers typically refer to indices
	// within the global vertex buffer. However, adding support for 16-bit index
	// buffers is trivial.

	using IndexBuffer16 = IMPL::IndexBuffer<std::uint16_t>;
	using IndexBuffer = IMPL::IndexBuffer<std::uint32_t>;
}