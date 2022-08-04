module;
#include <utility>
#include <cassert>
#include <DxDef.h>

module Brawler.GlobalTexturePageInfo;

namespace Brawler
{
	GlobalTexturePageInfo::GlobalTexturePageInfo(D3D12::Texture2D& globalTexture2D, D3D12::TextureCopyRegion&& pageRegion) :
		mGlobalTexture2DPtr(&globalTexture2D),
		mPageRegion(std::move(pageRegion))
	{}

	D3D12::Texture2D& GlobalTexturePageInfo::GetGlobalTexture2D() const
	{
		assert(mGlobalTexture2DPtr != nullptr);
		return *mGlobalTexture2DPtr;
	}

	const D3D12::TextureCopyRegion& GlobalTexturePageInfo::GetPageCopyRegion() const
	{
		return mPageRegion;
	}

	bool GlobalTexturePageInfo::ArePagesEquivalent(const GlobalTexturePageInfo& rhs) const
	{
		// For the two instances to refer to the same GlobalTexture page, they must be
		// referring to the same GlobalTexture.
		if (&(GetGlobalTexture2D()) != &(rhs.GetGlobalTexture2D()))
			return false;

		const CD3DX12_BOX& lhsCopyRegionBox{ mPageRegion.GetCopyRegionBox() };
		const CD3DX12_BOX& rhsCopyRegionBox{ rhs.mPageRegion.GetCopyRegionBox() };

		return (lhsCopyRegionBox.left == rhsCopyRegionBox.left && lhsCopyRegionBox.top == rhsCopyRegionBox.top && lhsCopyRegionBox.front == rhsCopyRegionBox.front &&
			lhsCopyRegionBox.right == rhsCopyRegionBox.right && lhsCopyRegionBox.bottom == rhsCopyRegionBox.bottom && lhsCopyRegionBox.back == rhsCopyRegionBox.back);
	}
}