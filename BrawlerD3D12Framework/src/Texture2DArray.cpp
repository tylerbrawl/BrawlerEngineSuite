module;
#include <optional>
#include <cassert>
#include "DxDef.h"

module Brawler.D3D12.Texture2DArray;
import Brawler.D3D12.GPUResourceInitializationInfo;

namespace
{
	template <typename BuilderType>
	constexpr Brawler::D3D12::GPUResourceInitializationInfo CreateGPUResourceInitializationInfo(const BuilderType& builder)
	{
		return Brawler::D3D12::GPUResourceInitializationInfo{
			.ResourceDesc{ builder.GetResourceDescription() },
			.InitialResourceState = builder.GetInitialResourceState(),
			.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT
		};
	}
}

namespace Brawler
{
	namespace D3D12
	{
		Texture2DArray::Texture2DArray(const Texture2DArrayBuilder& builder) :
			I_GPUResource(CreateGPUResourceInitializationInfo(builder)),
			mOptimizedClearValue(builder.GetOptimizedClearValue()),
			mInitMethod(builder.GetPreferredSpecialInitializationMethod())
		{}

		Texture2DArray::Texture2DArray(const RenderTargetTexture2DArrayBuilder& builder) :
			I_GPUResource(CreateGPUResourceInitializationInfo(builder)),
			mOptimizedClearValue(builder.GetOptimizedClearValue()),
			mInitMethod(builder.GetPreferredSpecialInitializationMethod())
		{}

		std::optional<D3D12_CLEAR_VALUE> Texture2DArray::GetOptimizedClearValue() const
		{
			return mOptimizedClearValue;
		}

		GPUResourceSpecialInitializationMethod Texture2DArray::GetPreferredSpecialInitializationMethod() const
		{
			return mInitMethod;
		}

		Texture2DArraySlice Texture2DArray::GetArraySlice(const std::uint32_t arraySliceIndex)
		{
			assert(arraySliceIndex < GetResourceDescription().DepthOrArraySize && "ERROR: An out-of-bounds array slice index was provided in a call to Texture2DArray::GetArraySlice()!");
			return Texture2DArraySlice{ *this, arraySliceIndex };
		}

		Texture2DArraySliceRange Texture2DArray::GetArraySliceRange(const std::uint32_t firstSliceIndex, const std::uint32_t numSlices)
		{
			assert((firstSliceIndex + numSlices) <= GetResourceDescription().DepthOrArraySize && "ERROR: An invalid range of array slices was provided in a call to Texture2DArray::GetArraySliceRange()!");
			return Texture2DArraySliceRange{ *this, firstSliceIndex, numSlices };
		}
	}
}