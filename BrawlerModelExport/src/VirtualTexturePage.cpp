module;
#include <utility>
#include <cstdint>
#include <cassert>
#include <DirectXMath/DirectXMath.h>
#include <DxDef.h>

module Brawler.VirtualTexturePage;

namespace Brawler
{
	VirtualTexturePage::VirtualTexturePage(VirtualTexturePageInitializationInfo&& initInfo) :
		mInitInfo(std::move(initInfo))
	{}

	D3D12::Texture2DSubResource VirtualTexturePage::GetTexture2DSubResource() const
	{
		return mInitInfo.TextureSubResource;
	}

	std::uint32_t VirtualTexturePage::GetStartingLogicalMipLevel() const
	{
		return mInitInfo.StartingLogicalMipLevel;
	}

	std::uint32_t VirtualTexturePage::GetLogicalMipLevelCount() const
	{
		return mInitInfo.LogicalMipLevelCount;
	}
}