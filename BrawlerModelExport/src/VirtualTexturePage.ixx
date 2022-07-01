module;
#include <cstdint>
#include <DirectXMath/DirectXMath.h>

export module Brawler.VirtualTexturePage;
import Brawler.D3D12.Texture2D;

export namespace Brawler
{
	struct VirtualTexturePageInitializationInfo
	{
		D3D12::Texture2DSubResource TextureSubResource;

		/// <summary>
		/// This field describes whether or not the relevant VirtualTexturePage represents a
		/// merged page; that is, it is the final page of a virtual texture which contains all
		/// of the mip levels which are smaller than the useful dimensions of a page.
		/// </summary>
		bool IsMergedPage;

		/// <summary>
		/// This field describes the coordinates of the relevant VirtualTexturePage within
		/// the relevant mip level of the virtual texture. The coordinates are zero-based
		/// and are based on the contents of the page with respect to their location in the
		/// virtual texture.
		/// 
		/// It is important to note that the units of PageCoordinates are in *pages*, and *NOT*
		/// in texels from the virtual texture.
		/// </summary>
		DirectX::XMUINT2 PageCoordinates;

		std::uint32_t StartingLogicalMipLevel;
		std::uint32_t LogicalMipLevelCount;
	};

	class VirtualTexturePage
	{
	public:
		VirtualTexturePage() = default;
		explicit VirtualTexturePage(VirtualTexturePageInitializationInfo&& initInfo);

		VirtualTexturePage(const VirtualTexturePage& rhs) = delete;
		VirtualTexturePage& operator=(const VirtualTexturePage& rhs) = delete;

		VirtualTexturePage(VirtualTexturePage&& rhs) noexcept = default;
		VirtualTexturePage& operator=(VirtualTexturePage&& rhs) noexcept = default;

		D3D12::Texture2DSubResource GetTexture2DSubResource() const;

		/// <summary>
		/// Returns the first logical mip level partially or completely represented by this VirtualTexturePage
		/// instance. Most virtual texture pages will only represent some or all of a single mip level of
		/// a virtual texture. However, mip maps smaller than the useful dimensions of a page are merged into
		/// a single page.
		/// </summary>
		/// <returns>
		/// The function returns the first logical mip level partially or completely represented by this 
		/// VirtualTexturePage instance.
		/// </returns>
		std::uint32_t GetStartingLogicalMipLevel() const;

		/// <summary>
		/// Returns the number of mip levels which this VirtualTexturePage either partially or completely
		/// represents. Most virtual texture pages will only represent some or all of a single mip level of
		/// a virtual texture. However, mip maps smaller than the useful dimensions of a page are merged into
		/// a single page.
		/// </summary>
		/// <returns>
		/// The function returns the number of mip levels which this VirtualTexturePage either partially or 
		/// completely represents.
		/// </returns>
		std::uint32_t GetLogicalMipLevelCount() const;

	private:
		VirtualTexturePageInitializationInfo mInitInfo;
	};
}