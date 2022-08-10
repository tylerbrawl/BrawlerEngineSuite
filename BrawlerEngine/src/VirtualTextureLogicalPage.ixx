module;
#include <cstdint>
#include <span>
#include <cassert>
#include <algorithm>
#include <DxDef.h>

export module Brawler.VirtualTextureLogicalPage;
import Brawler.VirtualTexture;
import Brawler.VirtualTextureMetadata;
import Brawler.D3D12.TextureCopyRegion;
import Brawler.D3D12.Texture2D;
import Brawler.GeneralHash;
import Brawler.Math.MathTypes;

export namespace Brawler
{
	struct VirtualTextureLogicalPage
	{
		VirtualTexture* VirtualTexturePtr;
		std::uint32_t LogicalMipLevel;
		std::uint32_t LogicalPageXCoordinate;
		std::uint32_t LogicalPageYCoordinate;

		D3D12::TextureCopyRegion GetIndirectionTextureCopyRegion() const
		{
			assert(VirtualTexturePtr != nullptr);
			
			const std::uint32_t firstMipLevelInCombinedPage = VirtualTexturePtr->GetVirtualTextureMetadata().GetFirstMipLevelInCombinedPage();
			const std::uint32_t clampedLogicalMipLevel = std::clamp<std::uint32_t>(LogicalMipLevel, 0, firstMipLevelInCombinedPage);
			const D3D12::Texture2DSubResource relevantSubResource{ VirtualTexturePtr->GetIndirectionTexture().GetSubResource(clampedLogicalMipLevel) };

			const bool isCombinedPage = (clampedLogicalMipLevel == firstMipLevelInCombinedPage);

			if (isCombinedPage)
			{
				// The combined page takes up one texel in the last mip level of the indirection texture.
				static constexpr CD3DX12_BOX BOX_FOR_COMBINED_PAGE_INDIRECTION_TEXEL{
					0,
					0,
					1,
					1
				};

				return D3D12::TextureCopyRegion{ relevantSubResource, BOX_FOR_COMBINED_PAGE_INDIRECTION_TEXEL };
			}
			else
			{
				// Every other logical virtual texture page takes up a 2x2 quad in the corresponding mip
				// level of the indirection texture.
				Math::UInt2 boxOriginCoordinates{ LogicalPageXCoordinate, LogicalPageYCoordinate };
				boxOriginCoordinates *= 2;

				const Math::UInt2 boxEndCoordinates{ boxOriginCoordinates + 2 };
				CD3DX12_BOX largePageIndirectionTexelBox{
					boxOriginCoordinates.GetX(),
					boxOriginCoordinates.GetY(),
					boxEndCoordinates.GetX(),
					boxEndCoordinates.GetY()
				};

				return D3D12::TextureCopyRegion{ relevantSubResource, std::move(largePageIndirectionTexelBox) };
			}
		}
	};
}

// ---------------------------------------------------------------------------------------------------------

export namespace std
{
	template <>
	struct hash<Brawler::VirtualTextureLogicalPage>
	{
		std::size_t operator()(const Brawler::VirtualTextureLogicalPage& key) const noexcept
		{
			const Brawler::GeneralHash<Brawler::VirtualTextureLogicalPage> hasher{};
			return hasher(key);
		}
	};
}

export namespace Brawler
{
	bool operator==(const VirtualTextureLogicalPage& lhs, const VirtualTextureLogicalPage& rhs)
	{
		return (lhs.VirtualTexturePtr == rhs.VirtualTexturePtr && lhs.LogicalMipLevel == rhs.LogicalMipLevel && lhs.LogicalPageXCoordinate == rhs.LogicalPageXCoordinate &&
			lhs.LogicalPageYCoordinate == rhs.LogicalPageYCoordinate);
	}
}