module;
#include <cassert>
#include "DxDef.h"

export module Brawler.D3D12.ConstantBufferView;

export namespace Brawler
{
	namespace D3D12
	{
		template <typename DataElementType>
		class ConstantBufferView
		{
		public:
			ConstantBufferView() = default;
			explicit ConstantBufferView(const D3D12_GPU_VIRTUAL_ADDRESS bufferAddress);

			void SetGPUAddress(const D3D12_GPU_VIRTUAL_ADDRESS bufferAddress);

			D3D12_CONSTANT_BUFFER_VIEW_DESC CreateCBVDescription() const;

		private:
			D3D12_GPU_VIRTUAL_ADDRESS mBufferAddress;
		};
	}
}

// -----------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <typename DataElementType>
		ConstantBufferView<DataElementType>::ConstantBufferView(const D3D12_GPU_VIRTUAL_ADDRESS bufferAddress) :
			mBufferAddress(bufferAddress)
		{}

		template <typename DataElementType>
		void ConstantBufferView<DataElementType>::SetGPUAddress(const D3D12_GPU_VIRTUAL_ADDRESS bufferAddress)
		{
			mBufferAddress = bufferAddress;
		}

		template <typename DataElementType>
		D3D12_CONSTANT_BUFFER_VIEW_DESC ConstantBufferView<DataElementType>::CreateCBVDescription() const
		{
			assert(mBufferAddress != 0 && "ERROR: An attempt was made to create a ConstantBufferView with a nullptr buffer address!");

			return D3D12_CONSTANT_BUFFER_VIEW_DESC{
				.BufferLocation = mBufferAddress,
				.SizeInBytes = sizeof(DataElementType)
			};
		}
	}
}