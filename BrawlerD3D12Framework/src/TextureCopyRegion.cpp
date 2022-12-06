module;
#include <algorithm>
#include <cassert>
#include "DxDef.h"

module Brawler.D3D12.TextureCopyRegion;

namespace Brawler
{
	namespace D3D12
	{
		TextureCopyRegion::TextureCopyRegion(const TextureSubResource& textureSubResource) :
			mResourcePtr(&(textureSubResource.GetGPUResource())),
			mSubResourceIndex(textureSubResource.GetSubResourceIndex()),
			mTargetBox()
		{
			// Calculate the CD3DX12_BOX which contains the entire sub-resource.
			const Brawler::D3D12_RESOURCE_DESC& resourceDesc{ mResourcePtr->GetResourceDescription() };

			std::size_t mipSlice = 0;
			std::size_t arraySlice = 0;
			std::size_t planeSlice = 0;

			const bool isTexture3DResource = (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE3D);
			const std::uint32_t arraySize = (isTexture3DResource ? 1 : resourceDesc.DepthOrArraySize);

			D3D12DecomposeSubresource(
				mSubResourceIndex,
				resourceDesc.MipLevels,
				arraySize,
				mipSlice,
				arraySlice,
				planeSlice
			);

			const std::uint32_t depth = (isTexture3DResource ? resourceDesc.DepthOrArraySize : 1);

			mTargetBox = CD3DX12_BOX{
				0,
				0,
				0,
				std::max<std::int32_t>(static_cast<std::int32_t>(resourceDesc.Width >> mipSlice), 1),
				std::max<std::int32_t>(static_cast<std::int32_t>(resourceDesc.Height >> mipSlice), 1),
				std::max<std::int32_t>(static_cast<std::int32_t>(depth >> mipSlice), 1)
			};
		}

		const I_GPUResource& TextureCopyRegion::GetGPUResource() const
		{
			assert(mResourcePtr != nullptr);
			return *mResourcePtr;
		}

		Brawler::D3D12Resource& TextureCopyRegion::GetD3D12Resource() const
		{
			return GetGPUResource().GetD3D12Resource();
		}

		std::uint32_t TextureCopyRegion::GetSubResourceIndex() const
		{
			return mSubResourceIndex;
		}

		CD3DX12_TEXTURE_COPY_LOCATION TextureCopyRegion::GetTextureCopyLocation() const
		{
			return CD3DX12_TEXTURE_COPY_LOCATION{ &GetD3D12Resource(), mSubResourceIndex };
		}

		const CD3DX12_BOX& TextureCopyRegion::GetCopyRegionBox() const
		{
			return mTargetBox;
		}
	}
}