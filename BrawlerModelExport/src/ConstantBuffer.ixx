module;
#include <array>
#include <optional>
#include "DxDef.h"

export module Brawler.D3D12.ConstantBuffer;
import Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.GPUResourceInitializationInfo;
import Util.Reflection;

namespace IMPL
{
	template <typename T>
	struct ArrayInfo
	{
		static constexpr bool IS_ARRAY = false;
	};

	template <typename ElementType_, std::size_t NumElements_>
	struct ArrayInfo<std::array<ElementType_, NumElements_>>
	{
		static constexpr bool IS_ARRAY = true;

		using ElementType = ElementType_;
		static constexpr std::size_t ELEMENT_COUNT = NumElements_;
	};

	template <typename T>
	struct HLSLFieldSolver
	{
	private:
		static constexpr std::size_t CBUFFER_PACKING_BOUNDARY = 16;

	public:
		template <std::size_t FieldIndex>
		consteval bool CheckField(std::size_t& currSize) const
		{
			using CurrFieldType = Util::Reflection::FieldType<T, FieldIndex>;

			// Pointers and references have no place in HLSL constant buffers, so we explicitly
			// disable them here. It is worth noting, however, that the reflection will cause
			// C-style arrays to decay back to pointers; this means that C-style arrays will
			// also fail this check.
			//
			// To work around this, we add explicit support for std::array below. (Besides,
			// std::arrays are infinitely superior to C-style arrays because they do not decay
			// into pointers and retain their size.)
			if constexpr (std::is_pointer_v<CurrFieldType>)
				return false;

			if constexpr (ArrayInfo<CurrFieldType>::IS_ARRAY)
			{
				// Arrays in HLSL are packed differently. They *MUST* start on a 16-byte boundary.
				// In addition, each element in the array is stored in a 16-byte vector. So, to
				// ensure that memory copies from CPU to GPU work correctly, each element within
				// an array should be 16 bytes large.

				// Ensure that the array starts on a 16-byte boundary.
				if (currSize % CBUFFER_PACKING_BOUNDARY != 0)
					return false;

				// Ensure that the elements in the array are exactly 16 bytes in size.
				//
				// TODO: What about arrays of elements which are larger than 16 bytes? How does
				// HLSL store these?
				const std::size_t elementSize = sizeof(ArrayInfo<CurrFieldType>::ElementType);
				if (elementSize != CBUFFER_PACKING_BOUNDARY)
					return false;

				currSize += (elementSize * ArrayInfo<CurrFieldType>::ELEMENT_COUNT);
			}
			else
			{
				// For all other data, HLSL will attempt to pack the values into 16-byte vectors,
				// with the restriction that no variable can straddle one of these vectors.
				const std::size_t currRemainder = currSize % CBUFFER_PACKING_BOUNDARY;

				// If the next field would cause us to move into a second vector, then we reject the
				// layout.
				if (currRemainder != 0 && currRemainder + sizeof(CurrFieldType) > CBUFFER_PACKING_BOUNDARY)
					return false;

				currSize += sizeof(CurrFieldType);
			}

			// Evaluate the next field.
			return CheckField<FieldIndex + 1>(currSize);
		}

		template <>
		consteval bool CheckField<Util::Reflection::GetFieldCount<T>()>(std::size_t& currSize) const
		{
			// We do not actually need the entire structure's size to be a multiple of
			// 16 bytes. DX12 requires us to align the size of constant buffers to
			// a 256-byte boundary, anyways.

			return true;
		}
	};

	template <typename T>
		requires Util::Reflection::IsReflectable<T>
	consteval bool IsHLSLConstantBufferCompatibleIMPL()
	{
		std::size_t currSize = 0;

		const HLSLFieldSolver<T> fieldSolver{};
		return fieldSolver.CheckField<0>(currSize);
	}

	template <typename T>
	concept IsHLSLConstantBufferCompatible = Util::Reflection::IsReflectable<T> && IsHLSLConstantBufferCompatibleIMPL<T>();
}

export namespace Brawler
{
	namespace D3D12
	{
		struct ConstantBufferInitializationInfo
		{
			D3D12_RESOURCE_FLAGS Flags;
			D3D12MA::ALLOCATION_DESC AllocationDesc;
			D3D12_RESOURCE_STATES InitialResourceState;
		};

		template <typename BufferT>
			requires IMPL::IsHLSLConstantBufferCompatible<BufferT>
		class ConstantBuffer final : public I_GPUResource
		{
		public:
			explicit ConstantBuffer(const ConstantBufferInitializationInfo& initInfo);

			ConstantBuffer(const ConstantBuffer& rhs) = delete;
			ConstantBuffer& operator=(const ConstantBuffer& rhs) = delete;

			ConstantBuffer(ConstantBuffer&& rhs) noexcept = default;
			ConstantBuffer& operator=(ConstantBuffer&& rhs) noexcept = default;

			std::optional<D3D12_CONSTANT_BUFFER_VIEW_DESC> CreateCBVDescription() const override;

		private:
			/// <summary>
			/// We need to cache this because CreateCBVDescription() is a const function, but
			/// ID3D12Resource::GetGPUVirtualAddress() isn't, for some reason.
			/// </summary>
			D3D12_GPU_VIRTUAL_ADDRESS mGPUAddress;
		};
	}
}

// -----------------------------------------------------------------------------------------------------

namespace
{
	template <typename BufferT>
		requires IMPL::IsHLSLConstantBufferCompatible<BufferT>
	constexpr Brawler::D3D12_RESOURCE_DESC CreateConstantBufferResourceDescription(const Brawler::D3D12::ConstantBufferInitializationInfo& initInfo)
	{
		return Brawler::D3D12_RESOURCE_DESC{
			.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER,
			.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
			.Width = sizeof(BufferT),
			.Height = 1,
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = DXGI_FORMAT_UNKNOWN,
			.SampleDesc{
				.Count = 1,
				.Quality = 0
			},
			.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
			.Flags = initInfo.Flags,
			.SamplerFeedbackMipRegion{}
		};
	}
}

namespace Brawler
{
	namespace D3D12
	{
		template <typename BufferT>
			requires IMPL::IsHLSLConstantBufferCompatible<BufferT>
		ConstantBuffer<BufferT>::ConstantBuffer(const ConstantBufferInitializationInfo& initInfo) :
			I_GPUResource(GPUResourceInitializationInfo{
				.ResourceDesc{ CreateConstantBufferResourceDescription<BufferT>(initInfo) },
				.AllocationDesc{ initInfo.AllocationDesc },
				.InitialResourceState{ initInfo.InitialResourceState }
			}),
			mGPUAddress(GetD3D12Resource().GetGPUVirtualAddress())
		{}

		template <typename BufferT>
			requires IMPL::IsHLSLConstantBufferCompatible<BufferT>
		std::optional<D3D12_CONSTANT_BUFFER_VIEW_DESC> ConstantBuffer<BufferT>::CreateCBVDescription() const
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{
				.BufferLocation = mGPUAddress,
				.SizeInBytes = sizeof(BufferT)
			};

			return std::optional<D3D12_CONSTANT_BUFFER_VIEW_DESC>{ std::move(cbvDesc) };
		}
	}
}