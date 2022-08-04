module;

export module Brawler.GlobalTexturePageInfo;
import Brawler.D3D12.Texture2D;
import Brawler.D3D12.TextureCopyRegion;

export namespace Brawler
{
	class GlobalTexturePageInfo
	{
	public:
		GlobalTexturePageInfo() = default;
		GlobalTexturePageInfo(D3D12::Texture2D& globalTexture2D, D3D12::TextureCopyRegion&& pageRegion);

		GlobalTexturePageInfo(const GlobalTexturePageInfo& rhs) = delete;
		GlobalTexturePageInfo& operator=(const GlobalTexturePageInfo& rhs) = delete;

		GlobalTexturePageInfo(GlobalTexturePageInfo&& rhs) noexcept = default;
		GlobalTexturePageInfo& operator=(GlobalTexturePageInfo&& rhs) noexcept = default;

		D3D12::Texture2D& GetGlobalTexture2D() const;
		const D3D12::TextureCopyRegion& GetPageCopyRegion() const;

		bool ArePagesEquivalent(const GlobalTexturePageInfo& rhs) const;

	private:
		D3D12::Texture2D* mGlobalTexture2DPtr;
		D3D12::TextureCopyRegion mPageRegion;
	};
}