module;
#include "DxDef.h"

export module Brawler.D3D12.TextureCopyRegion;
import Brawler.D3D12.TextureSubResource;
import Brawler.D3D12.I_GPUResource;

export namespace Brawler
{
	namespace D3D12
	{
		class TextureCopyRegion
		{
		public:
			TextureCopyRegion() = default;
			TextureCopyRegion(const TextureSubResource& textureSubResource, CD3DX12_BOX&& targetBox);

			TextureCopyRegion(const TextureCopyRegion& rhs) = default;
			TextureCopyRegion& operator=(const TextureCopyRegion& rhs) = default;

			TextureCopyRegion(TextureCopyRegion&& rhs) noexcept = default;
			TextureCopyRegion& operator=(TextureCopyRegion&& rhs) noexcept = default;

			const I_GPUResource& GetGPUResource() const;

			Brawler::D3D12Resource& GetD3D12Resource() const;
			std::uint32_t GetSubResourceIndex() const;

			CD3DX12_TEXTURE_COPY_LOCATION GetTextureCopyLocation() const;
			const CD3DX12_BOX& GetCopyRegionBox() const;

		private:
			const I_GPUResource* mResourcePtr;
			std::uint32_t mSubResourceIndex;
			CD3DX12_BOX mTargetBox;
		};
	}
}