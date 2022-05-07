module;
#include <cassert>
#include "DxDef.h"

export module Brawler.D3D12.ConstantBufferView;
import Brawler.D3D12.I_BufferSubAllocation;

export namespace Brawler
{
	namespace D3D12
	{
		template <typename DataElementType>
		class ConstantBufferView
		{
		public:
			ConstantBufferView() = default;
			explicit ConstantBufferView(const I_BufferSubAllocation& subAllocation, const std::size_t offsetInElements = 0);

			D3D12_CONSTANT_BUFFER_VIEW_DESC CreateCBVDescription() const;

			const I_BufferSubAllocation& GetBufferSubAllocation() const;
			std::size_t GetOffsetFromSubAllocationStart() const;

		private:
			const I_BufferSubAllocation* mSubAllocationPtr;
			std::size_t mOffsetInElements;
		};
	}
}

// -----------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <typename DataElementType>
		ConstantBufferView<DataElementType>::ConstantBufferView(const I_BufferSubAllocation& subAllocation, const std::size_t offsetInElements) :
			mSubAllocationPtr(&subAllocation),
			mOffsetInElements(offsetInElements)
		{}

		template <typename DataElementType>
		D3D12_CONSTANT_BUFFER_VIEW_DESC ConstantBufferView<DataElementType>::CreateCBVDescription() const
		{
			assert(mSubAllocationPtr != nullptr && "ERROR: An attempt was made to create a ConstantBufferView without giving it an I_BufferSubAllocation to reference!");

			return D3D12_CONSTANT_BUFFER_VIEW_DESC{
				.BufferLocation = mSubAllocationPtr->GetGPUVirtualAddress() + GetOffsetFromSubAllocationStart(),
				.SizeInBytes = sizeof(DataElementType)
			};
		}

		template <typename DataElementType>
		const I_BufferSubAllocation& ConstantBufferView<DataElementType>::GetBufferSubAllocation() const
		{
			assert(mSubAllocationPtr != nullptr && "ERROR: An attempt was made to create a ConstantBufferView without giving it an I_BufferSubAllocation to reference!");
			return *mSubAllocationPtr;
		}

		template <typename DataElementType>
		std::size_t ConstantBufferView<DataElementType>::GetOffsetFromSubAllocationStart() const
		{
			return (mOffsetInElements * sizeof(DataElementType));
		}
	}
}