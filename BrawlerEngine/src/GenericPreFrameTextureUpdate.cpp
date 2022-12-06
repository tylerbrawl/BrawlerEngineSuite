module;
#include <span>
#include <vector>
#include <DxDef.h>

module Brawler.GenericPreFrameTextureUpdate;

namespace Brawler
{
	GenericPreFrameTextureUpdate::GenericPreFrameTextureUpdate(D3D12::TextureSubResource textureSubResource) :
		mDestSubResource(std::move(textureSubResource)),

		// Use the relevant constructor of D3D12::TextureCopyRegion to construct the
		// correct CD3DX12_BOX for us.
		mCopyRegionBox(D3D12::TextureCopyRegion{ mDestSubResource }.GetCopyRegionBox()),

		mDataArr()
	{}

	GenericPreFrameTextureUpdate::GenericPreFrameTextureUpdate(D3D12::TextureSubResource textureSubResource, CD3DX12_BOX copyRegionBox) :
		mDestSubResource(std::move(textureSubResource)),
		mCopyRegionBox(std::move(copyRegionBox)),
		mDataArr()
	{}

	D3D12::TextureSubResource& GenericPreFrameTextureUpdate::GetDestinationTextureSubResource()
	{
		return mDestSubResource;
	}

	const D3D12::TextureSubResource& GenericPreFrameTextureUpdate::GetDestinationTextureSubResource() const
	{
		return mDestSubResource;
	}

	D3D12::TextureCopyRegion GenericPreFrameTextureUpdate::GetTextureCopyRegion() const
	{
		return D3D12::TextureCopyRegion{ mDestSubResource, mCopyRegionBox };
	}

	std::span<const std::byte> GenericPreFrameTextureUpdate::GetDataByteSpan() const
	{
		return std::span<const std::byte>{ mDataArr };
	}

	std::vector<std::byte> GenericPreFrameTextureUpdate::ExtractDataByteArray()
	{
		return std::move(mDataArr);
	}

	std::size_t GenericPreFrameTextureUpdate::GetUpdateRegionSize() const
	{
		return mDataArr.size();
	}
}