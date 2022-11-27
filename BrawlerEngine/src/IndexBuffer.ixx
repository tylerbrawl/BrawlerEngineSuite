module;
#include <cstddef>
#include <memory>

export module Brawler.IndexBuffer;
import Brawler.D3D12.BufferResource;
import Brawler.D3D12.BindlessSRVAllocation;
import Brawler.D3D12.AlignedByteAddressBufferSubAllocation;

namespace Brawler 
{
	namespace IMPL
	{
		template <std::integral IndexType>
		class IndexBuffer
		{
		private:
			using SubAllocationType = D3D12::DynamicAlignedByteAddressBufferSubAllocation<alignof(IndexType)>;

		public:
			IndexBuffer() = default;
			explicit IndexBuffer(const std::size_t numIndices);

			IndexBuffer(const IndexBuffer& rhs) = delete;
			IndexBuffer& operator=(const IndexBuffer& rhs) = delete;

			IndexBuffer(IndexBuffer&& rhs) noexcept = default;
			IndexBuffer& operator=(IndexBuffer&& rhs) noexcept = default;

		private:
			std::unique_ptr<D3D12::BufferResource> mIndexBufferPtr;
			SubAllocationType mIndexBufferSubAllocation;
			D3D12::BindlessSRVAllocation mBindlessSRVAllocation;
		};
	}
}

export namespace Brawler
{
	// We use 32-bit indices because our index buffers typically refer to indices
	// within the global vertex buffer.

	using IndexBuffer16 = IMPL::IndexBuffer<std::uint16_t>;
	using IndexBuffer = IMPL::IndexBuffer<std::uint32_t>;
}