module;
#include <cassert>
#include "DxDef.h"

export module Brawler.D3D12.TextureCopyRegion;
import Brawler.D3D12.TextureSubResource;
import Brawler.D3D12.I_GPUResource;
import Util.General;

export namespace Brawler
{
	namespace D3D12
	{
		class TextureCopyRegion
		{
		public:
			TextureCopyRegion() = default;

			template <typename T>
				requires std::is_same_v<std::decay_t<T>, CD3DX12_BOX>
			TextureCopyRegion(const TextureSubResource& textureSubResource, T&& targetBox);

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

// ---------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <typename T>
			requires std::is_same_v<std::decay_t<T>, CD3DX12_BOX>
		TextureCopyRegion::TextureCopyRegion(const TextureSubResource& textureSubResource, T&& targetBox) :
			mResourcePtr(&(textureSubResource.GetGPUResource())),
			mSubResourceIndex(textureSubResource.GetSubResourceIndex()),
			mTargetBox(std::forward<T>(targetBox))
		{
			if constexpr (Util::General::IsDebugModeEnabled())
			{
				assert(mTargetBox.right >= mTargetBox.left && "ERROR: An invalid CD3DX12_BOX instance was provided!");
				assert(mTargetBox.bottom >= mTargetBox.top && "ERROR: An invalid CD3DX12_BOX instance was provided!");
				assert(mTargetBox.back >= mTargetBox.front && "ERROR: An invalid CD3DX12_BOX instance was provided!");

				const Brawler::D3D12_RESOURCE_DESC& resourceDesc{ mResourcePtr->GetResourceDescription() };

				if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE1D || resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D)
					assert(mTargetBox.front == 0 && mTargetBox.back == 1 && "ERROR: For a 1D or 2D texture resource, the front field of a CD3DX12_BOX instance must be 0, and the back field must be 1!");

				if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE1D)
					assert(mTargetBox.top == 0 && mTargetBox.bottom == 1 && "ERROR: For a 1D texture resource, the top field of a CD3DX12_BOX instance must be 0, and the bottom field must be 1!");
			}
		}
	}
}