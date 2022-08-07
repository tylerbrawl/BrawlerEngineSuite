module;
#include "DxDef.h"

module Brawler.D3D12.TextureCopyRegion;

namespace Brawler
{
	namespace D3D12
	{
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