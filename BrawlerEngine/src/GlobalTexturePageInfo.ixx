module;
#include <cstdint>
#include <DxDef.h>
#include <DirectXMath/DirectXMath.h>

export module Brawler.GlobalTexturePageInfo;
import Brawler.D3D12.Texture2D;
import Brawler.D3D12.TextureCopyRegion;
import Brawler.GlobalTextureFormatInfo;
import Brawler.Math.MathTypes;
import Brawler.GlobalTexturePageIdentifier;

export namespace Brawler
{
	class GlobalTexturePageInfo
	{
	public:
		template <DXGI_FORMAT Format>
		struct InitInfo
		{
			D3D12::Texture2D& GlobalTexture2D;
			Math::UInt2 PageCoordinates;
			std::uint32_t GlobalTextureID;
		};
		
	public:
		GlobalTexturePageInfo() = default;

		template <DXGI_FORMAT Format>
		explicit GlobalTexturePageInfo(const InitInfo<Format>& initInfo);

		GlobalTexturePageInfo(const GlobalTexturePageInfo& rhs) = delete;
		GlobalTexturePageInfo& operator=(const GlobalTexturePageInfo& rhs) = delete;

		GlobalTexturePageInfo(GlobalTexturePageInfo&& rhs) noexcept = default;
		GlobalTexturePageInfo& operator=(GlobalTexturePageInfo&& rhs) noexcept = default;

		D3D12::Texture2D& GetGlobalTexture2D() const;
		const D3D12::TextureCopyRegion& GetPageCopyRegion() const;
		GlobalTexturePageIdentifier GetPageIdentifier() const;

		bool ArePagesEquivalent(const GlobalTexturePageInfo& rhs) const;

	private:
		D3D12::Texture2D* mGlobalTexture2DPtr;
		Math::UInt2 mPageCoordinates;
		std::uint32_t mGlobalTextureID;
		D3D12::TextureCopyRegion mPageRegion;
	};
}

// ------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <DXGI_FORMAT Format>
	GlobalTexturePageInfo::GlobalTexturePageInfo(const InitInfo<Format>& initInfo) :
		mGlobalTexture2DPtr(&(initInfo.GlobalTexture2D)),
		mPageCoordinates(initInfo.PageCoordinates),
		mGlobalTextureID(initInfo.GlobalTextureID),
		mPageRegion()
	{
		static constexpr std::uint32_t PADDED_PAGE_DIMENSIONS = GlobalTextureFormatInfo<Format>::PADDED_PAGE_DIMENSIONS;

		const Brawler::Math::UInt2 pageTexelCoordinatesBegin{ mPageCoordinates * PADDED_PAGE_DIMENSIONS };
		const Brawler::Math::UInt2 pageTexelCoordinatesEnd{ pageTexelCoordinatesBegin + PADDED_PAGE_DIMENSIONS };

		CD3DX12_BOX copyRegionBox{
			pageTexelCoordinatesBegin.GetX(),
			pageTexelCoordinatesBegin.GetY(),
			pageTexelCoordinatesEnd.GetX(),
			pageTexelCoordinatesEnd.GetY()
		};

		mPageRegion = D3D12::TextureCopyRegion{ mGlobalTexture2DPtr->GetSubResource(0), std::move(copyRegionBox) };
	}
}