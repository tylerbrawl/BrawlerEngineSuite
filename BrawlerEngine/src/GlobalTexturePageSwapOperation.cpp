module;
#include <optional>
#include <cassert>

module Brawler.GlobalTexturePageSwapOperation;

namespace Brawler
{
	GlobalTexturePageSwapOperation::GlobalTexturePageSwapOperation(InitInfo&& initInfo) :
		mInitInfo(std::move(initInfo))
	{}

	D3D12::Texture2D& GlobalTexturePageSwapOperation::GetGlobalTexture() const
	{
		return mInitInfo.GlobalTexture;
	}

	const GlobalTextureReservedPage& GlobalTexturePageSwapOperation::GetReplacementPage() const
	{
		return mInitInfo.NewReservedPage;
	}

	bool GlobalTexturePageSwapOperation::IsReplacingOlderPage() const
	{
		return mInitInfo.OldReservedPage.has_value();
	}

	const GlobalTextureReservedPage& GlobalTexturePageSwapOperation::GetPreviousPage() const
	{
		assert(IsReplacingOlderPage());
		return *(mInitInfo.OldReservedPage);
	}

	std::uint16_t GlobalTexturePageSwapOperation::GetGlobalTextureXPageCoordinates() const
	{
		return mInitInfo.GlobalTextureXPageCoordinates;
	}

	std::uint16_t GlobalTexturePageSwapOperation::GetGlobalTextureYPageCoordinates() const
	{
		return mInitInfo.GlobalTextureYPageCoordinates;
	}
}