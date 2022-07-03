module;
#include <utility>
#include <cstdint>
#include <cassert>
#include <optional>
#include <DxDef.h>

module Brawler.VirtualTexturePage;

namespace Brawler
{
	VirtualTexturePage::VirtualTexturePage(VirtualTexturePageInitializationInfo&& initInfo) :
		mInitInfo(std::move(initInfo)),
		mHCompressedData()
	{}

	D3D12::Texture2DSubResource VirtualTexturePage::GetTexture2DSubResource() const
	{
		return mInitInfo.TextureSubResource;
	}

	DirectX::XMUINT2 VirtualTexturePage::GetPageCoordinates() const
	{
		return mInitInfo.PageCoordinates;
	}

	std::uint32_t VirtualTexturePage::GetStartingLogicalMipLevel() const
	{
		return mInitInfo.StartingLogicalMipLevel;
	}

	std::uint32_t VirtualTexturePage::GetLogicalMipLevelCount() const
	{
		return mInitInfo.LogicalMipLevelCount;
	}

	void VirtualTexturePage::AssignCompressedDataBufferReservation(D3D12::BufferSubAllocationReservationHandle&& hReservation)
	{
		mHCompressedData = std::move(hReservation);
	}

	bool VirtualTexturePage::HasCompressedDataBufferReservation() const
	{
		return mHCompressedData.has_value();
	}

	D3D12::BufferSubAllocationReservationHandle VirtualTexturePage::RevokeCompressedDataBufferReservation()
	{
		assert(HasCompressedDataBufferReservation());

		D3D12::BufferSubAllocationReservationHandle hReservation{ std::move(*mHCompressedData) };
		mHCompressedData.reset();

		return hReservation;
	}
}