module;
#include <optional>
#include "DxDef.h"

module Brawler.D3D12.DepthStencilTexture;
import Brawler.D3D12.GPUResourceInitializationInfo;

namespace
{
	Brawler::D3D12::GPUResourceInitializationInfo CreateGPUResourceInitializationInfo(const Brawler::D3D12::DepthStencilTextureBuilder& builder)
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
		DepthStencilTexture::DepthStencilTexture(const DepthStencilTextureBuilder& builder) :
			I_GPUResource(CreateGPUResourceInitializationInfo(builder)),
			mOptimizedClearValue(builder.GetOptimizedClearValue()),
			mInitMethod(builder.GetPreferredSpecialInitializationMethod())
		{}

		std::optional<D3D12_CLEAR_VALUE> DepthStencilTexture::GetOptimizedClearValue() const
		{
			return mOptimizedClearValue;
		}

		GPUResourceSpecialInitializationMethod DepthStencilTexture::GetPreferredSpecialInitializationMethod() const
		{
			return mInitMethod;
		}
	}
}