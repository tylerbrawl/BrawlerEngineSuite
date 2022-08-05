module;
#include <utility>
#include <cassert>
#include <DxDef.h>

module Brawler.GlobalTexturePageInfo;

namespace Brawler
{
	D3D12::Texture2D& GlobalTexturePageInfo::GetGlobalTexture2D() const
	{
		assert(mGlobalTexture2DPtr != nullptr);
		return *mGlobalTexture2DPtr;
	}

	const D3D12::TextureCopyRegion& GlobalTexturePageInfo::GetPageCopyRegion() const
	{
		return mPageRegion;
	}

	GlobalTexturePageIdentifier GlobalTexturePageInfo::GetPageIdentifier() const
	{
		// These conversions should all be safe; after all, the R8G8B8A8 indirection
		// textures need to store these values.
		assert(mGlobalTextureID <= std::numeric_limits<std::uint8_t>::max());
		assert(mPageCoordinates.GetX() <= std::numeric_limits<std::uint8_t>::max());
		assert(mPageCoordinates.GetY() <= std::numeric_limits<std::uint8_t>::max());

		return GlobalTexturePageIdentifier{
			.GlobalTextureID = static_cast<std::uint8_t>(mGlobalTextureID),
			.GlobalTexturePageXCoordinate = static_cast<std::uint8_t>(mPageCoordinates.GetX()),
			.GlobalTexturePageYCoordinate = static_cast<std::uint8_t>(mPageCoordinates.GetY())
		};
	}

	bool GlobalTexturePageInfo::ArePagesEquivalent(const GlobalTexturePageInfo& rhs) const
	{
		return (mGlobalTextureID == rhs.mGlobalTextureID && mPageCoordinates == rhs.mPageCoordinates);
	}
}