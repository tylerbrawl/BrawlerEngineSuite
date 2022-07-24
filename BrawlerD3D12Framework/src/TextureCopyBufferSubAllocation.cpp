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
			InitializeTextureInfo(textureSubResource.GetResourceDescription(), textureSubResource.GetSubResourceIndex());
		}

		TextureCopyBufferSubAllocation::TextureCopyBufferSubAllocation(const TextureCopyRegion& textureCopyRegion) :
			mTextureInfo()
		{
			// Create a base D3D12_RESOURCE_DESC from the associated I_GPUResource instance. Then,
			// modify its contents to reflect the data which we will be copying.
			Brawler::D3D12_RESOURCE_DESC resourceDesc{ textureCopyRegion.GetGPUResource().GetResourceDescription() };
			const CD3DX12_BOX& copyRegionBox{ textureCopyRegion.GetCopyRegionBox() };
			
			resourceDesc.Width = (static_cast<std::uint64_t>(copyRegionBox.right) - static_cast<std::uint64_t>(copyRegionBox.left));
			resourceDesc.Height = (resourceDesc.Dimension != D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE1D ? (copyRegionBox.bottom - copyRegionBox.top) : 1);
			resourceDesc.DepthOrArraySize = (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE3D ? (copyRegionBox.back - copyRegionBox.front) : 1);

			// Regardless of the sub-resource referred to by the TextureCopyRegion, when calling
			// ID3D12Device8::GetCopyableFootprints1(), we always specify 0 as the sub-resource index.
			// That way, it always uses the dimensions which we specified above.
			InitializeTextureInfo(resourceDesc, 0);
		}

		TextureCopyBufferSubAllocation::TextureCopyBufferSubAllocation(const Util::D3D12::CopyableFootprint& footprint) :
			mTextureInfo()
		{
			mTextureInfo.Footprint = footprint.Layout;
			mTextureInfo.RowCount = footprint.NumRows;
			mTextureInfo.UnpaddedRowSizeInBytes = footprint.RowSizeInBytes;

			mTextureInfo.TotalBytesRequired = ((footprint.Layout.Footprint.RowPitch * (footprint.NumRows - 1)) + mTextureInfo.UnpaddedRowSizeInBytes);
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

		void TextureCopyBufferSubAllocation::InitializeTextureInfo(const Brawler::D3D12_RESOURCE_DESC& resourceDesc, const std::uint32_t subResourceIndex)
		{
			Util::Engine::GetD3D12Device().GetCopyableFootprints1(
				&resourceDesc,
				subResourceIndex,
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