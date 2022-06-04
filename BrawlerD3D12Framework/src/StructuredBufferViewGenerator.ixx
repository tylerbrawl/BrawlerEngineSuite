module;
#include <cassert>
#include "DxDef.h"

export module Brawler.D3D12.StructuredBufferSubAllocation:StructuredBufferViewGenerator;
import :StructuredBufferElementRange;
import Brawler.D3D12.GPUResourceViews;
import Brawler.D3D12.RootDescriptors;
import Brawler.OptionalRef;
import Brawler.D3D12.GenericBufferSnapshots;

namespace Brawler
{
	namespace D3D12
	{
		template <typename T>
		concept D3D12ConstantBufferDataPlacementAligned = (sizeof(T) % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0);
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		template <typename DerivedClass, typename DataElementType>
		class StructuredBufferViewGenerator
		{
		public:
			StructuredBufferViewGenerator() = default;

			RootConstantBufferView CreateRootConstantBufferView(const std::uint32_t elementIndex) const requires D3D12ConstantBufferDataPlacementAligned<DataElementType>;
			ConstantBufferView<DataElementType> CreateTableConstantBufferView(const std::uint32_t elementIndex) const requires D3D12ConstantBufferDataPlacementAligned<DataElementType>;

			RootShaderResourceView CreateRootShaderResourceView() const;
			StructuredBufferShaderResourceView CreateTableShaderResourceView() const;
			StructuredBufferShaderResourceView CreateTableShaderResourceView(const StructuredBufferElementRange& viewedElementsRange) const;

			RootUnorderedAccessView CreateRootUnorderedAccessView() const;
			StructuredBufferUnorderedAccessView CreateTableUnorderedAccessView() const;
			StructuredBufferUnorderedAccessView CreateTableUnorderedAccessView(const StructuredBufferElementRange& viewedElementsRange) const;

		private:
			StructuredBufferElementRange ConvertElementRangeToAbsoluteUnits(const StructuredBufferElementRange& rangeInRelativeUnits) const;

			D3D12_BUFFER_SRV CreateBufferSRVDescription(const StructuredBufferElementRange& rangeInRelativeUnits) const;
			D3D12_BUFFER_UAV CreateBufferUAVDescription(const StructuredBufferElementRange& rangeInRelativeUnits) const;

			DerivedClass& GetDerivedClass();
			const DerivedClass& GetDerivedClass() const;
		};
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <typename DerivedClass, typename DataElementType>
		RootConstantBufferView StructuredBufferViewGenerator<DerivedClass, DataElementType>::CreateRootConstantBufferView(const std::uint32_t elementIndex) const requires D3D12ConstantBufferDataPlacementAligned<DataElementType>
		{
			assert(static_cast<std::size_t>(elementIndex) < GetDerivedClass().GetElementCount());

			return RootConstantBufferView{ GetDerivedClass().GetGPUVirtualAddress() + (sizeof(DataElementType) * elementIndex) };
		}

		template <typename DerivedClass, typename DataElementType>
		ConstantBufferView<DataElementType> StructuredBufferViewGenerator<DerivedClass, DataElementType>::CreateTableConstantBufferView(const std::uint32_t elementIndex) const requires D3D12ConstantBufferDataPlacementAligned<DataElementType>
		{
			assert(static_cast<std::size_t>(elementIndex) < GetDerivedClass().GetElementCount());

			return ConstantBufferView<DataElementType>{ D3D12_CONSTANT_BUFFER_VIEW_DESC{
				.BufferLocation = GetDerivedClass().GetGPUVirtualAddress() + (sizeof(DataElementType) * elementIndex),
				.SizeInBytes = sizeof(DataElementType)
			} };
		}

		template <typename DerivedClass, typename DataElementType>
		RootShaderResourceView StructuredBufferViewGenerator<DerivedClass, DataElementType>::CreateRootShaderResourceView() const
		{
			return RootShaderResourceView{ GetDerivedClass().GetGPUVirtualAddress() };
		}

		template <typename DerivedClass, typename DataElementType>
		StructuredBufferShaderResourceView StructuredBufferViewGenerator<DerivedClass, DataElementType>::CreateTableShaderResourceView() const
		{
			return StructuredBufferShaderResourceView{ GetDerivedClass().GetBufferResource(), CreateBufferSRVDescription(StructuredBufferElementRange{
				.FirstElement = 0,
				.NumElements = GetDerivedClass().GetElementCount()
			}) };
		}

		template <typename DerivedClass, typename DataElementType>
		StructuredBufferShaderResourceView StructuredBufferViewGenerator<DerivedClass, DataElementType>::CreateTableShaderResourceView(const StructuredBufferElementRange& viewedElementsRange) const
		{
			assert(viewedElementsRange.FirstElement + viewedElementsRange.NumElements <= GetDerivedClass().GetElementCount() && "ERROR: An invalid StructuredBufferElementRange was provided when trying to make an SRV for a StructuredBuffer!");
			return StructuredBufferShaderResourceView{ GetDerivedClass().GetBufferResource(), CreateBufferSRVDescription(viewedElementsRange) };
		}

		template <typename DerivedClass, typename DataElementType>
		RootUnorderedAccessView StructuredBufferViewGenerator<DerivedClass, DataElementType>::CreateRootUnorderedAccessView() const
		{
			return RootUnorderedAccessView{ GetDerivedClass().GetGPUVirtualAddress() };
		}

		template <typename DerivedClass, typename DataElementType>
		StructuredBufferUnorderedAccessView StructuredBufferViewGenerator<DerivedClass, DataElementType>::CreateTableUnorderedAccessView() const
		{
			D3D12_BUFFER_UAV viewDesc{ CreateBufferUAVDescription(StructuredBufferElementRange{
				.FirstElement = 0,
				.NumElements = GetDerivedClass().GetElementCount()
			}) };

			StructuredBufferUnorderedAccessView bufferUAV{ GetDerivedClass().GetBufferResource(), std::move(viewDesc) };

			const Brawler::OptionalRef<const UAVCounterSnapshot> uavCounterSnapshot{ GetDerivedClass().GetUAVCounter() };

			if (uavCounterSnapshot.HasValue()) [[unlikely]]
				bufferUAV.SetUAVCounterResource(uavCounterSnapshot->GetD3D12Resource());

			return bufferUAV;
		}

		template <typename DerivedClass, typename DataElementType>
		StructuredBufferUnorderedAccessView StructuredBufferViewGenerator<DerivedClass, DataElementType>::CreateTableUnorderedAccessView(const StructuredBufferElementRange& viewedElementsRange) const
		{
			assert(viewedElementsRange.FirstElement + viewedElementsRange.NumElements <= GetDerivedClass().GetElementCount() && "ERROR: An invalid StructuredBufferElementRange was provided when trying to make a UAV for a StructuredBuffer!");

			StructuredBufferUnorderedAccessView bufferUAV{ GetDerivedClass().GetBufferResource(), CreateBufferUAVDescription(viewedElementsRange) };

			Brawler::OptionalRef<UAVCounterSnapshot> uavCounterSnapshot{ GetDerivedClass().GetUAVCounter() };

			if (uavCounterSnapshot.HasValue()) [[unlikely]]
				bufferUAV.SetUAVCounterResource(uavCounterSnapshot->GetD3D12Resource());

			return bufferUAV;
		}

		template <typename DerivedClass, typename DataElementType>
		StructuredBufferElementRange StructuredBufferViewGenerator<DerivedClass, DataElementType>::ConvertElementRangeToAbsoluteUnits(const StructuredBufferElementRange& rangeInRelativeUnits) const
		{
			return StructuredBufferElementRange{
				.FirstElement = (GetDerivedClass().GetOffsetFromBufferStart() / sizeof(DataElementType)) + rangeInRelativeUnits.FirstElement,
				.NumElements = rangeInRelativeUnits.NumElements
			};
		}

		template <typename DerivedClass, typename DataElementType>
		D3D12_BUFFER_SRV StructuredBufferViewGenerator<DerivedClass, DataElementType>::CreateBufferSRVDescription(const StructuredBufferElementRange& rangeInRelativeUnits) const
		{
			const StructuredBufferElementRange absoluteViewedElementsRange{ ConvertElementRangeToAbsoluteUnits(rangeInRelativeUnits) };

			return D3D12_BUFFER_SRV{
				.FirstElement = absoluteViewedElementsRange.FirstElement,
				.NumElements = static_cast<std::uint32_t>(absoluteViewedElementsRange.NumElements),
				.StructureByteStride = sizeof(DataElementType),
				.Flags = D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_NONE
			};
		}

		template <typename DerivedClass, typename DataElementType>
		D3D12_BUFFER_UAV StructuredBufferViewGenerator<DerivedClass, DataElementType>::CreateBufferUAVDescription(const StructuredBufferElementRange& rangeInRelativeUnits) const
		{
			const StructuredBufferElementRange absoluteViewedElementsRange{ ConvertElementRangeToAbsoluteUnits(rangeInRelativeUnits) };
			const Brawler::OptionalRef<const UAVCounterSnapshot> uavCounterSnapshot{ GetDerivedClass().GetUAVCounter() };

			return D3D12_BUFFER_UAV{
				.FirstElement = absoluteViewedElementsRange.FirstElement,
				.NumElements = static_cast<std::uint32_t>(absoluteViewedElementsRange.NumElements),
				.StructureByteStride = sizeof(DataElementType),

				// Even if we specify nullptr for the UAV counter in ID3D12Device::CreateUnorderedAccessView(), we still
				// need CounterOffsetInBytes to be 0, as the D3D12 API requires it, for some reason.
				.CounterOffsetInBytes = (uavCounterSnapshot.HasValue() ? uavCounterSnapshot->GetOffsetFromBufferStart() : 0),

				.Flags = D3D12_BUFFER_UAV_FLAGS::D3D12_BUFFER_UAV_FLAG_NONE
			};
		}

		template <typename DerivedClass, typename DataElementType>
		DerivedClass& StructuredBufferViewGenerator<DerivedClass, DataElementType>::GetDerivedClass()
		{
			return *(static_cast<DerivedClass*>(this));
		}

		template <typename DerivedClass, typename DataElementType>
		const DerivedClass& StructuredBufferViewGenerator<DerivedClass, DataElementType>::GetDerivedClass() const
		{
			return *(static_cast<const DerivedClass*>(this));
		}
	}
}