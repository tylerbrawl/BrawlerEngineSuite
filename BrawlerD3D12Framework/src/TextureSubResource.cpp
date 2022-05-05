module;
#include <cassert>
#include "DxDef.h"

module Brawler.D3D12.TextureSubResource;
import Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.Texture2D;

namespace Brawler
{
	namespace D3D12
	{
		TextureSubResource::TextureSubResource(const Texture2D& texture2D, const std::uint32_t subResourceIndex) :
			mResourcePtr(&texture2D),
			mSubResourceIndex(subResourceIndex)
		{}

		const I_GPUResource& TextureSubResource::GetGPUResource() const
		{
			assert(mResourcePtr != nullptr);
			return *mResourcePtr;
		}

		Brawler::D3D12Resource& TextureSubResource::GetD3D12Resource() const
		{
			assert(mResourcePtr != nullptr);
			return mResourcePtr->GetD3D12Resource();
		}

		std::uint32_t TextureSubResource::GetSubResourceIndex() const
		{
			return mSubResourceIndex;
		}

		const Brawler::D3D12_RESOURCE_DESC& TextureSubResource::GetResourceDescription() const
		{
			assert(mResourcePtr != nullptr);
			return mResourcePtr->GetResourceDescription();
		}
	}
}