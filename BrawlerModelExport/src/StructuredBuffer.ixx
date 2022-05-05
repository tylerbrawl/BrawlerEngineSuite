module;
#include <cassert>
#include <cstddef>
#include <optional>
#include "DxDef.h"

export module Brawler.D3D12.StructuredBuffer;
import Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.GPUResourceInitializationInfo;
import Brawler.CompositeEnum;

namespace Brawler
{
	namespace D3D12
	{
		// UAV counters are 32-bit atomic values.
		static constexpr std::size_t UAV_COUNTER_SIZE = sizeof(std::uint32_t);
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		enum class StructuredBufferFlags
		{
			NONE,
			
			/// <summary>
			/// Indicates that this StructuredBuffer should store a UAV counter. If this flag is
			/// set, then the UAV counter is placed at the beginning of the buffer, and the buffer
			/// is made big enough to hold both this counter and all of its intended data.
			/// </summary>
			INCLUDE_UAV_COUNTER,

			COUNT_OR_ERROR
		};

		struct StructuredBufferInitializationInfo
		{
			std::size_t NumElements;
			Brawler::CompositeEnum<StructuredBufferFlags> BufferFlags;
			D3D12_RESOURCE_FLAGS ResourceFlags;
			D3D12MA::ALLOCATION_DESC AllocationDesc;
			D3D12_RESOURCE_STATES InitialResourceState;
		};

		template <typename BufferEntryT>
		class StructuredBuffer final : public I_GPUResource
		{
		public:
			explicit StructuredBuffer(const StructuredBufferInitializationInfo& initInfo);

			StructuredBuffer(const StructuredBuffer& rhs) = delete;
			StructuredBuffer& operator=(const StructuredBuffer& rhs) = delete;

			StructuredBuffer(StructuredBuffer&& rhs) noexcept = default;
			StructuredBuffer& operator=(StructuredBuffer&& rhs) noexcept = default;

			std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> CreateSRVDescription() const override;
			std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> CreateUAVDescription() const override;

			/// <summary>
			/// Retrieves the number of elements which this StructuredBuffer can hold.
			/// </summary>
			/// <returns>
			/// The function returns the number of elements which this StructuredBuffer can
			/// hold.
			/// </returns>
			std::size_t GetSize() const;

		private:
			Brawler::CompositeEnum<StructuredBufferFlags> mFlags;
		};
	}
}

// --------------------------------------------------------------------------------------------------------------------

namespace
{
	template <typename BufferEntryT>
	constexpr Brawler::D3D12_RESOURCE_DESC CreateStructuredBufferResourceDescription(const Brawler::D3D12::StructuredBufferInitializationInfo& initInfo)
	{
		// Only allow D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS and D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE as
		// special flags.
		assert(initInfo.ResourceFlags & ~(D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0 && "ERROR: Invalid D3D12_RESOURCE_FLAGS were provided when creating a StructuredBuffer!");

		return Brawler::D3D12_RESOURCE_DESC{
			.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER,
			.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,

			// Make room for the UAV counter inside of this resource if we are to include one.
			.Width = ((initInfo.NumElements * sizeof(BufferEntryT)) + (((initInfo.BufferFlags & Brawler::D3D12::StructuredBufferFlags::INCLUDE_UAV_COUNTER) != 0) ? Brawler::D3D12::UAV_COUNTER_SIZE : 0)),

			.Height = 1,
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN,
			.SampleDesc{
				.Count = 1,
				.Quality = 0
			},
			.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
			.Flags = initInfo.ResourceFlags,
			.SamplerFeedbackMipRegion{}
		};
	}
}

namespace Brawler
{
	namespace D3D12
	{
		template <typename BufferEntryT>
		StructuredBuffer<BufferEntryT>::StructuredBuffer(const StructuredBufferInitializationInfo& initInfo) :
			I_GPUResource(GPUResourceInitializationInfo{
				.ResourceDesc{ CreateStructuredBufferResourceDescription<BufferEntryT>(initInfo) },
				.AllocationDesc{ initInfo.AllocationDesc },
				.InitialResourceState{ initInfo.InitialResourceState }
			}),
			mFlags(initInfo.BufferFlags)
		{}

		template <typename BufferEntryT>
		std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> StructuredBuffer<BufferEntryT>::CreateSRVDescription() const
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{
				.Format = GetResourceDescription().Format,
				.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_BUFFER,
				.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
				.Buffer{
					.FirstElement = 0,
					.NumElements = static_cast<std::uint32_t>(GetSize()),
					.StructureByteStride = sizeof(BufferEntryT),
					.Flags = D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_NONE
				}
			};

			return std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC>{ std::move(srvDesc) };
		}

		template <typename BufferEntryT>
		std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> StructuredBuffer<BufferEntryT>::CreateUAVDescription() const
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{
				.Format = GetResourceDescription().Format,
				.ViewDimension = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_BUFFER,
				.Buffer{
					.FirstElement = 0,
					.NumElements = static_cast<std::uint32_t>(GetSize()),
					.StructureByteStride = sizeof(BufferEntryT),
					.CounterOffsetInBytes = 0
				}
			};

			return std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC>{ std::move(uavDesc) };
		}

		template <typename BufferEntryT>
		std::size_t StructuredBuffer<BufferEntryT>::GetSize() const
		{
			std::size_t dataElementsWidth = GetResourceDescription().Width;

			if ((mFlags & StructuredBufferFlags::INCLUDE_UAV_COUNTER) != 0)
				dataElementsWidth -= UAV_COUNTER_SIZE;
			
			return (dataElementsWidth / sizeof(BufferEntryT));
		}
	}
}