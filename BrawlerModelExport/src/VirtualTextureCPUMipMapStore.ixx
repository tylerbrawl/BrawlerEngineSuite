module;
#include <cstdint>
#include <vector>
#include <DxDef.h>

export module Brawler.VirtualTextureCPUDataStore:VirtualTextureCPUMipMapStore;
import :VirtualTextureCPUPageStore;
import Brawler.VirtualTexturePageFilterMode;
import Brawler.D3D12.Texture2D;

export namespace Brawler
{
	template <DXGI_FORMAT TextureFormat, VirtualTexturePageFilterMode FilterMode>
	class VirtualTextureCPUMipMapStore
	{
	private:
		using VirtualTexturePageRow = std::vector<VirtualTextureCPUPageStore<TextureFormat, FilterMode>>;

	public:
		VirtualTextureCPUMipMapStore() = default;
		VirtualTextureCPUMipMapStore(const D3D12::Texture2D& srcTexture, const std::uint32_t startingLogicalMipLevel);

		VirtualTextureCPUMipMapStore(const VirtualTextureCPUMipMapStore& rhs) = delete;
		VirtualTextureCPUMipMapStore& operator=(const VirtualTextureCPUMipMapStore& rhs) = delete;

		VirtualTextureCPUMipMapStore(VirtualTextureCPUMipMapStore&& rhs) noexcept = default;
		VirtualTextureCPUMipMapStore& operator=(VirtualTextureCPUMipMapStore&& rhs) noexcept = default;



	private:
		std::uint32_t mStartingLogicalMipLevel;
	};
}