module;
#include <cassert>
#include "DxDef.h"

module Brawler.D3D12.TextureCopyRegion;
import Util.General;

namespace Brawler
{
	namespace D3D12
	{
		TextureCopyRegion::TextureCopyRegion(const TextureSubResource& textureSubResource, CD3DX12_BOX&& targetBox) :
			mResourcePtr(&(textureSubResource.GetGPUResource())),
			mSubResourceIndex(textureSubResource.GetSubResourceIndex()),
			mTargetBox(std::move(targetBox))
		{
			if constexpr (Util::General::IsDebugModeEnabled())
			{
				assert(mTargetBox.right >= mTargetBox.left && "ERROR: An invalid CD3DX12_BOX instance was provided!");
				assert(mTargetBox.bottom >= mTargetBox.top && "ERROR: An invalid CD3DX12_BOX instance was provided!");
				assert(mTargetBox.back >= mTargetBox.front && "ERROR: An invalid CD3DX12_BOX instance was provided!");

				const Brawler::D3D12_RESOURCE_DESC& resourceDesc{ mResourcePtr->GetResourceDescription() };

				if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE1D || resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D)
					assert(mTargetBox.front == 0 && mTargetBox.back == 1 && "ERROR: For a 1D or 2D texture resource, the front field of a CD3DX12_BOX instance must be 0, and the back field must be 1!");

				if(resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE1D)
					assert(mTargetBox.top == 0 && mTargetBox.bottom == 1 && "ERROR: For a 1D texture resource, the top field of a CD3DX12_BOX instance must be 0, and the bottom field must be 1!");
			}
		}

		I_GPUResource& TextureCopyRegion::GetGPUResource()
		{
			assert(mResourcePtr != nullptr);
			return *mResourcePtr;
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