module;
#include "DxDef.h"

module Brawler.D3D12.TextureCopyBufferSubAllocation;
import Brawler.D3D12.TextureSubResource;
import Util.Engine;

namespace Brawler
{
	namespace D3D12
	{
		TextureCopyBufferSubAllocation::TextureCopyBufferSubAllocation(const TextureSubResource& textureSubResource) :
			mTextureInfo()
		{
			InitializeTextureInfo(textureSubResource);
		}

		std::size_t TextureCopyBufferSubAllocation::GetSubAllocationSize() const
		{
			return mTextureInfo.TotalBytesRequired;
		}

		std::size_t TextureCopyBufferSubAllocation::GetRequiredDataPlacementAlignment() const
		{
			// Texture data for the sake of texture copy operations must be placed into a buffer at an
			// alignment of D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT from the start of the buffer.
			//
			// (This is not to be confused with D3D12_TEXTURE_DATA_PITCH_ALIGNMENT, which specifies the
			// alignment from the start of the sub-allocation at which rows of texture data must start.)
			return D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
		}

		void TextureCopyBufferSubAllocation::OnReservationAssigned()
		{
			// Update the offset from the start of the buffer.
			mTextureInfo.Footprint.Offset = GetOffsetFromBufferStart();
		}
		
		std::uint32_t TextureCopyBufferSubAllocation::GetRowCount() const
		{
			return mTextureInfo.RowCount;
		}

		std::size_t TextureCopyBufferSubAllocation::GetRowSizeInBytes() const
		{
			return mTextureInfo.UnpaddedRowSizeInBytes;
		}

		CD3DX12_TEXTURE_COPY_LOCATION TextureCopyBufferSubAllocation::GetBufferTextureCopyLocation() const
		{
			return CD3DX12_TEXTURE_COPY_LOCATION{ &GetD3D12Resource(), mTextureInfo.Footprint };
		}

		void TextureCopyBufferSubAllocation::InitializeTextureInfo(const TextureSubResource& textureSubResource)
		{
			Util::Engine::GetD3D12Device().GetCopyableFootprints1(
				&(textureSubResource.GetResourceDescription()),
				textureSubResource.GetSubResourceIndex(),
				1,
				0,
				&(mTextureInfo.Footprint),
				&(mTextureInfo.RowCount),
				&(mTextureInfo.UnpaddedRowSizeInBytes),
				&(mTextureInfo.TotalBytesRequired)
			);
		}
	}
}