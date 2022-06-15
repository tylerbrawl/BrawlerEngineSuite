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
			explicit ConstantBufferView(D3D12_CONSTANT_BUFFER_VIEW_DESC&& cbvDesc);

			const D3D12_CONSTANT_BUFFER_VIEW_DESC& GetCBVDescription() const;

		private:
			D3D12_CONSTANT_BUFFER_VIEW_DESC mCBVDesc;
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
			mCBVDesc(D3D12_CONSTANT_BUFFER_VIEW_DESC{
				.BufferLocation = subAllocation.GetGPUVirtualAddress() + (offsetInElements * sizeof(DataElementType)),
				.SizeInBytes = sizeof(DataElementType)
			})
		{}

		template <typename DataElementType>
		ConstantBufferView<DataElementType>::ConstantBufferView(D3D12_CONSTANT_BUFFER_VIEW_DESC&& cbvDesc) :
			mCBVDesc(std::move(cbvDesc))
		{}

		template <typename DataElementType>
		const D3D12_CONSTANT_BUFFER_VIEW_DESC& ConstantBufferView<DataElementType>::GetCBVDescription() const
		{
			return mCBVDesc;
		}
	}
}