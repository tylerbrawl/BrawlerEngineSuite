module;
#include <cstdint>
#include <cassert>
#include "DxDef.h"

module Brawler.D3D12.Texture2DArray;

namespace Brawler
{
	namespace D3D12
	{
		Texture2DArraySlice::Texture2DArraySlice(Texture2DArray& tex2DArr, const std::uint32_t arraySliceIndex) :
			mTexArrayPtr(&tex2DArr),
			mArraySliceIndex(arraySliceIndex)
		{}

		Texture2DArray& Texture2DArraySlice::GetTexture2DArray()
		{
			assert(mTexArrayPtr != nullptr);
			return *mTexArrayPtr;
		}

		const Texture2DArray& Texture2DArraySlice::GetTexture2DArray() const
		{
			assert(mTexArrayPtr != nullptr);
			return *mTexArrayPtr;
		}

		Texture2DArraySubResource Texture2DArraySlice::GetSubResource(const std::uint32_t mipSlice)
		{
			const Brawler::D3D12_RESOURCE_DESC& resourceDesc{ GetTexture2DArray().GetResourceDescription() };
			assert(mipSlice < resourceDesc.MipLevels && "ERROR: An out-of-bounds mip level index was specified in a call to Texture2DArraySlice::GetSubResource()!");
			assert(D3D12GetFormatPlaneCount(&(Util::Engine::GetD3D12Device()), resourceDesc.Format) == 1 && "ERROR: Congratulations! You found a multi-planar texture format which isn't meant for depth/stencil textures. Have fun figuring out how to create descriptors for it~");

			const std::uint32_t desiredSubResourceIndex = D3D12CalcSubresource(
				mipSlice,
				mArraySliceIndex,
				0,
				resourceDesc.MipLevels,
				resourceDesc.DepthOrArraySize
			);

			return Texture2DArraySubResource{ GetTexture2DArray(), desiredSubResourceIndex };
		}
	}
}