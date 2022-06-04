module;
#include <array>
#include <span>
#include "DxDef.h"

export module Brawler.D3D12.ConstantBufferSubAllocation;
import Brawler.D3D12.I_BufferSubAllocation;
import Brawler.D3D12.I_BufferSnapshot;
import Util.HLSL;
import Util.Math;
import Brawler.D3D12.RootDescriptors;
import Brawler.D3D12.GPUResourceViews;

namespace Brawler
{
	namespace D3D12
	{
		template <typename T>
		concept HLSLConstantBufferCompatible = (Util::HLSL::IsHLSLConstantBufferAligned<T>() && (sizeof(T) <= Util::Math::KilobytesToBytes(64)));
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		template <typename T>
			requires HLSLConstantBufferCompatible<T>
		class ConstantBufferSubAllocation;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		template <typename T>
			requires HLSLConstantBufferCompatible<T>
		class ConstantBufferSnapshot final : public I_BufferSnapshot
		{
		public:
			explicit ConstantBufferSnapshot(const ConstantBufferSubAllocation<T>& cbSubAllocation);

			ConstantBufferSnapshot(const ConstantBufferSnapshot& rhs) = default;
			ConstantBufferSnapshot& operator=(const ConstantBufferSnapshot& rhs) = default;

			ConstantBufferSnapshot(ConstantBufferSnapshot&& rhs) noexcept = default;
			ConstantBufferSnapshot& operator=(ConstantBufferSnapshot&& rhs) noexcept = default;

			RootConstantBufferView CreateRootConstantBufferView() const;
			ConstantBufferView<T> CreateTableConstantBufferView() const;

			RootShaderResourceView CreateRootShaderResourceView() const;
			StructuredBufferShaderResourceView CreateTableShaderResourceView() const;

		private:
			D3D12_BUFFER_SRV CreateBufferSRVDescription() const;
		};
	}
}

// -----------------------------------------------------------------------------------------------------------------------------

export namespace Brawler
{
	namespace D3D12
	{
		template <typename T>
			requires HLSLConstantBufferCompatible<T>
		class ConstantBufferSubAllocation final : public I_BufferSubAllocation
		{
		public:
			ConstantBufferSubAllocation() = default;

			ConstantBufferSubAllocation(const ConstantBufferSubAllocation& rhs) = delete;
			ConstantBufferSubAllocation& operator=(const ConstantBufferSubAllocation& rhs) = delete;

			ConstantBufferSubAllocation(ConstantBufferSubAllocation&& rhs) noexcept = default;
			ConstantBufferSubAllocation& operator=(ConstantBufferSubAllocation&& rhs) noexcept = default;

			std::size_t GetSubAllocationSize() const override;
			std::size_t GetRequiredDataPlacementAlignment() const override;

			template <typename U>
				requires std::is_same_v<std::decay_t<T>, std::decay_t<U>>
			void WriteConstantBufferData(U&& cbData) const;

			RootConstantBufferView CreateRootConstantBufferView() const;
			ConstantBufferView<T> CreateTableConstantBufferView() const;

			RootShaderResourceView CreateRootShaderResourceView() const;
			StructuredBufferShaderResourceView CreateTableShaderResourceView() const;

		private:
			D3D12_BUFFER_SRV CreateBufferSRVDescription() const;
		};
	}
}

// -----------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <typename T>
			requires HLSLConstantBufferCompatible<T>
		ConstantBufferSnapshot<T>::ConstantBufferSnapshot(const ConstantBufferSubAllocation<T>& cbSubAllocation) :
			I_BufferSnapshot(cbSubAllocation)
		{}

		template <typename T>
			requires HLSLConstantBufferCompatible<T>
		RootConstantBufferView ConstantBufferSnapshot<T>::CreateRootConstantBufferView() const
		{
			return RootConstantBufferView{ GetGPUVirtualAddress() };
		}

		template <typename T>
			requires HLSLConstantBufferCompatible<T>
		ConstantBufferView<T> ConstantBufferSnapshot<T>::CreateTableConstantBufferView() const
		{
			return ConstantBufferView<T>{ D3D12_CONSTANT_BUFFER_VIEW_DESC{
				.BufferLocation = GetGPUVirtualAddress(),
				.SizeInBytes = sizeof(T)
			} };
		}

		template <typename T>
			requires HLSLConstantBufferCompatible<T>
		RootShaderResourceView ConstantBufferSnapshot<T>::CreateRootShaderResourceView() const
		{
			return RootShaderResourceView{ GetGPUVirtualAddress() };
		}

		template <typename T>
			requires HLSLConstantBufferCompatible<T>
		StructuredBufferShaderResourceView ConstantBufferSnapshot<T>::CreateTableShaderResourceView() const
		{
			return StructuredBufferShaderResourceView{ GetBufferResource(), CreateBufferSRVDescription() };
		}

		template <typename T>
			requires HLSLConstantBufferCompatible<T>
		D3D12_BUFFER_SRV ConstantBufferSnapshot<T>::CreateBufferSRVDescription() const
		{
			return D3D12_BUFFER_SRV{
				.FirstElement = (GetOffsetFromBufferStart() / sizeof(T)),
				.NumElements = 1,
				.StructureByteStride = sizeof(T),
				.Flags = D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_NONE
			};
		}
	}
}

// -----------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <typename T>
			requires HLSLConstantBufferCompatible<T>
		std::size_t ConstantBufferSubAllocation<T>::GetSubAllocationSize() const
		{
			return sizeof(T);
		}

		template <typename T>
			requires HLSLConstantBufferCompatible<T>
		std::size_t ConstantBufferSubAllocation<T>::GetRequiredDataPlacementAlignment() const
		{
			// DirectX 12 requires that data which is to be used as a constant buffer MUST be
			// placed at a 256-byte alignment from the start of the buffer.

			return D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
		}

		template <typename T>
			requires HLSLConstantBufferCompatible<T>
		template <typename U>
			requires std::is_same_v<std::decay_t<T>, std::decay_t<U>>
		void ConstantBufferSubAllocation<T>::WriteConstantBufferData(U&& cbData) const
		{
			const std::span<const T> srcDataSpan{ std::addressof(cbData), 1 };
			WriteToBuffer(srcDataSpan, 0);
		}

		template <typename T>
			requires HLSLConstantBufferCompatible<T>
		RootConstantBufferView ConstantBufferSubAllocation<T>::CreateRootConstantBufferView() const
		{
			return RootConstantBufferView{ *this };
		}

		template <typename T>
			requires HLSLConstantBufferCompatible<T>
		ConstantBufferView<T> ConstantBufferSubAllocation<T>::CreateTableConstantBufferView() const
		{
			return ConstantBufferView<T>{ *this };
		}

		template <typename T>
			requires HLSLConstantBufferCompatible<T>
		RootShaderResourceView ConstantBufferSubAllocation<T>::CreateRootShaderResourceView() const
		{
			return RootShaderResourceView{ *this };
		}

		template <typename T>
			requires HLSLConstantBufferCompatible<T>
		StructuredBufferShaderResourceView ConstantBufferSubAllocation<T>::CreateTableShaderResourceView() const
		{
			return StructuredBufferShaderResourceView{ GetBufferResource(), CreateBufferSRVDescription() };
		}

		template <typename T>
			requires HLSLConstantBufferCompatible<T>
		D3D12_BUFFER_SRV ConstantBufferSubAllocation<T>::CreateBufferSRVDescription() const
		{
			return D3D12_BUFFER_SRV{
				// For the sake of the driver, we can "pretend" that the entire buffer consists only
				// of structures of size sizeof(T). Then, we know that the index of the element which 
				// we are looking for is (GetOffsetFromBufferStart() / sizeof(T)).
				.FirstElement = (GetOffsetFromBufferStart() / sizeof(T)),

				.NumElements = 1,
				.StructureByteStride = sizeof(T),
				.Flags = D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_NONE
			};
		}
	}
}