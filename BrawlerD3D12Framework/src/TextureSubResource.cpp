module;
#include <cassert>
#include "DxDef.h"

module Brawler.D3D12.TextureSubResource;
import Brawler.D3D12.I_GPUResource;

namespace Brawler
{
	namespace D3D12
	{
		TextureSubResource::TextureSubResource(const std::uint32_t subResourceIndex) :
			mSubResourceIndex(subResourceIndex)
		{}
		
		Brawler::D3D12Resource& TextureSubResource::GetD3D12Resource() const
		{
			return GetGPUResource().GetD3D12Resource();
		}

		std::uint32_t TextureSubResource::GetSubResourceIndex() const
		{
			return mSubResourceIndex;
		}

		const Brawler::D3D12_RESOURCE_DESC& TextureSubResource::GetResourceDescription() const
		{
			return GetGPUResource().GetResourceDescription();
		}
	}
}