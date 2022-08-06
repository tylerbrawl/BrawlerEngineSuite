module;
#include <cstdint>
#include <cassert>
#include "DxDef.h"

module Brawler.D3D12.Texture2DArray;

namespace Brawler
{
	namespace D3D12
	{
		Texture2DArraySubResource::Texture2DArraySubResource(Texture2DArray& tex2DArr, const std::uint32_t subResourceIndex) :
			TextureSubResource(subResourceIndex),
			mTexArrayPtr(&tex2DArr)
		{}

		I_GPUResource& Texture2DArraySubResource::GetGPUResource()
		{
			assert(mTexArrayPtr != nullptr);
			return *mTexArrayPtr;
		}

		const I_GPUResource& Texture2DArraySubResource::GetGPUResource() const
		{
			assert(mTexArrayPtr != nullptr);
			return *mTexArrayPtr;
		}

		Texture2DArray& Texture2DArraySubResource::GetTexture2DArray()
		{
			assert(mTexArrayPtr != nullptr);
			return *mTexArrayPtr;
		}

		const Texture2DArray& Texture2DArraySubResource::GetTexture2DArray() const
		{
			assert(mTexArrayPtr != nullptr);
			return *mTexArrayPtr;
		}

		Texture2DArraySubResource::SubResourceInfo Texture2DArraySubResource::GetSubResourceInfo() const
		{
			const Brawler::D3D12_RESOURCE_DESC& resourceDesc{ GetTexture2DArray().GetResourceDescription() };
			SubResourceInfo subResourceInfo{};

			D3D12DecomposeSubresource(
				GetSubResourceIndex(),
				static_cast<std::uint32_t>(resourceDesc.MipLevels),
				static_cast<std::uint32_t>(resourceDesc.DepthOrArraySize),
				subResourceInfo.MipLevel,
				subResourceInfo.ArraySlice,
				subResourceInfo.PlaneSlice
			);

			return subResourceInfo;
		}
	}
}