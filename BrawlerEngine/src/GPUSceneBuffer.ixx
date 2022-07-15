module;
#include <cstddef>
#include <memory>
#include <optional>
#include <DxDef.h>

export module Brawler.GPUSceneBuffer;
import Brawler.D3D12.BufferResource;
import Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.ByteAddressBufferSubAllocation;
import Brawler.D3D12.StructuredBufferSubAllocation;
import Brawler.D3D12.BindlessSRVAllocation;
import Brawler.D3D12.BufferResourceInitializationInfo;
import Brawler.GPUSceneTypes;

namespace Brawler
{
	template <typename ElementType, std::size_t NumElements>
	struct GPUSceneBufferSubAllocationInfo
	{
		static_assert(sizeof(ElementType) != sizeof(ElementType), "ERROR: An invalid GPU scene buffer data type was detected!");
	};

	template <typename T, std::size_t NumElements, DXGI_FORMAT SRVFormat>
		requires (SRVFormat != DXGI_FORMAT::DXGI_FORMAT_UNKNOWN)
	struct PrimitiveTypeGPUSceneBufferSubAllocationInfo
	{
		using SubAllocationType = D3D12::ByteAddressBufferSubAllocation<(NumElements * sizeof(T))>;
		using SnapshotType = D3D12::ByteAddressBufferSnapshot<(NumElements * sizeof(T))>;

		static std::optional<SubAllocationType> CreateSubAllocation(D3D12::BufferResource& bufferResource)
		{
			std::optional<SubAllocationType> createdSubAllocation{ bufferResource.CreateBufferSubAllocation<SubAllocationType>() };
			return createdSubAllocation;
		}

		static constexpr D3D12_SHADER_RESOURCE_VIEW_DESC DEFAULT_SRV_DESC{
			.Format = SRVFormat,
			.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_BUFFER,
			.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Buffer{
				.FirstElement = 0,
				.NumElements = NumElements,
				.StructureByteStride = 0,
				.Flags = D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_NONE
			}
		};
	};

	template <std::size_t NumElements>
	struct GPUSceneBufferSubAllocationInfo<std::int32_t, NumElements> : public PrimitiveTypeGPUSceneBufferSubAllocationInfo<std::int32_t, NumElements, DXGI_FORMAT::DXGI_FORMAT_R32_SINT>
	{};

	template <std::size_t NumElements>
	struct GPUSceneBufferSubAllocationInfo<std::uint32_t, NumElements> : public PrimitiveTypeGPUSceneBufferSubAllocationInfo<std::uint32_t, NumElements, DXGI_FORMAT::DXGI_FORMAT_R32_UINT>
	{};

	template <std::size_t NumElements>
	struct GPUSceneBufferSubAllocationInfo<float, NumElements> : public PrimitiveTypeGPUSceneBufferSubAllocationInfo<float, NumElements, DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT>
	{};

	template <typename T, std::size_t NumElements>
		requires requires ()
	{
		D3D12::StructuredBufferSubAllocation<T, NumElements>{};
	}
	struct GPUSceneBufferSubAllocationInfo<T, NumElements>
	{
		using SubAllocationType = D3D12::StructuredBufferSubAllocation<T, NumElements>;
		using SnapshotType = D3D12::StructuredBufferSnapshot<T>;

		static std::optional<SubAllocationType> CreateSubAllocation(D3D12::BufferResource& bufferResource)
		{
			std::optional<SubAllocationType> createdSubAllocation{ bufferResource.CreateBufferSubAllocation<SubAllocationType>() };
			return createdSubAllocation;
		}

		static constexpr D3D12_SHADER_RESOURCE_VIEW_DESC DEFAULT_SRV_DESC{
			.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN,
			.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_BUFFER,
			.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Buffer{
				.FirstElement = 0,
				.NumElements = NumElements,
				.StructureByteStride = sizeof(T),
				.Flags = D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_NONE
			}
		};
	};
}

export namespace Brawler
{
	template <typename ElementType, std::size_t NumElements>
	class GPUSceneBuffer
	{
	private:
		using SubAllocationType = typename GPUSceneBufferSubAllocationInfo<ElementType, NumElements>::SubAllocationType;
		using SnapshotType = typename GPUSceneBufferSubAllocationInfo<ElementType, NumElements>::SnapshotType;

	public:
		GPUSceneBuffer() = default;

		GPUSceneBuffer(const GPUSceneBuffer& rhs) = delete;
		GPUSceneBuffer& operator=(const GPUSceneBuffer& rhs) = delete;

		GPUSceneBuffer(GPUSceneBuffer&& rhs) noexcept = default;
		GPUSceneBuffer& operator=(GPUSceneBuffer&& rhs) noexcept = default;

		void Initialize();

	private:
		std::unique_ptr<D3D12::BufferResource> mBufferPtr;
		SubAllocationType mBufferSubAllocation;
		D3D12::BindlessSRVAllocation mBindlessAllocation;
	};
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename ElementType, std::size_t NumElements>
	void GPUSceneBuffer<ElementType, NumElements>::Initialize()
	{
		constexpr D3D12::BufferResourceInitializationInfo BUFFER_INITIALIZATION_INFO{
			.SizeInBytes = (sizeof(ElementType) * NumElements),
			.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT
		};

		mBufferPtr = std::make_unique<D3D12::BufferResource>(BUFFER_INITIALIZATION_INFO);

		std::optional<SubAllocationType> bufferSubAllocation{ GPUSceneBufferSubAllocationInfo<ElementType, NumElements>::CreateSubAllocation(*mBufferPtr) };
		assert(bufferSubAllocation.has_value());

		mBufferSubAllocation = std::move(*bufferSubAllocation);

		// Create the BindlessSRVAllocation here. The order in which GPUSceneBuffer instances are initialized
		// should match the order in which the buffers should appear in the GPUResourceDescriptorHeap.
		//
		// Specifically, the shaders will assume that a given GPUSceneBuffer is located at a constant index
		// within the bindless SRV segment of the GPUResourceDescriptorHeap. We need the calls to
		// GPUSceneBuffer::Initialize() to match this ordering.
		D3D12_SHADER_RESOURCE_VIEW_DESC bindlessSRVDesc{ GPUSceneBufferSubAllocationInfo<ElementType, NumElements>::DEFAULT_SRV_DESC };
		mBindlessAllocation = static_cast<D3D12::I_GPUResource&>(*mBufferPtr).CreateBindlessSRV(std::move(bindlessSRVDesc));
	}
}