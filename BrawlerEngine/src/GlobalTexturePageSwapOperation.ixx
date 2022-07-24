module;
#include <optional>
#include <DxDef.h>

export module Brawler.GlobalTexturePageSwapOperation;
import Brawler.D3D12.Texture2D;
import Brawler.VirtualTextureLogicalPage;
import Brawler.GlobalTextureReservedPage;
import Brawler.D3D12.TextureCopyBufferSubAllocation;

export namespace Brawler
{
	class GlobalTexturePageSwapOperation
	{
	public:
		// NOTE: We copy the GlobalTextureReservedPage instances by value because we do not want
		// to end up with different data if something changes after the GlobalTexturePageSwapOperation
		// instance is created.
		struct InitInfo
		{
			D3D12::Texture2D& GlobalTexture;
			std::uint32_t PageDimensions;
			GlobalTextureReservedPage NewReservedPage;
			std::optional<GlobalTextureReservedPage> OldReservedPage;
			std::uint16_t GlobalTextureXPageCoordinates;
			std::uint16_t GlobalTextureYPageCoordinates;
			std::uint8_t GlobalTextureDescriptionBufferIndex;
		};

	public:
		explicit GlobalTexturePageSwapOperation(InitInfo&& initInfo);

		GlobalTexturePageSwapOperation(const GlobalTexturePageSwapOperation& rhs) = delete;
		GlobalTexturePageSwapOperation& operator=(const GlobalTexturePageSwapOperation& rhs) = delete;

		GlobalTexturePageSwapOperation(GlobalTexturePageSwapOperation&& rhs) noexcept = default;
		GlobalTexturePageSwapOperation& operator=(GlobalTexturePageSwapOperation&& rhs) noexcept = default;

		void AssignGlobalTextureCopySubAllocation(D3D12::TextureCopyBufferSubAllocation&& subAllocation);

		D3D12::TextureCopyBufferSubAllocation& GetGlobalTextureCopySubAllocation();
		const D3D12::TextureCopyBufferSubAllocation& GetGlobalTextureCopySubAllocation() const;

		D3D12::Texture2D& GetGlobalTexture() const;
		const GlobalTextureReservedPage& GetReplacementPage() const;

		bool IsReplacingOlderPage() const;
		const GlobalTextureReservedPage& GetPreviousPage() const;

		std::uint16_t GetGlobalTextureXPageCoordinates() const;
		std::uint16_t GetGlobalTextureYPageCoordinates() const;

		std::uint8_t GetGlobalTextureDescriptionBufferIndex() const;

		CD3DX12_BOX GetGlobalTextureCopyRegionBox() const;

	private:
		InitInfo mInitInfo;
		D3D12::TextureCopyBufferSubAllocation mGlobalTextureCopySubAllocation;
	};
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------

// Defining these functions in a module implementation unit is causing weird circular-dependency-like
// behavior in the MSVC.

namespace Brawler
{
	GlobalTexturePageSwapOperation::GlobalTexturePageSwapOperation(InitInfo&& initInfo) :
		mInitInfo(std::move(initInfo)),
		mGlobalTextureCopySubAllocation()
	{}

	void GlobalTexturePageSwapOperation::AssignGlobalTextureCopySubAllocation(D3D12::TextureCopyBufferSubAllocation&& subAllocation)
	{
		mGlobalTextureCopySubAllocation = std::move(subAllocation);
	}

	D3D12::TextureCopyBufferSubAllocation& GlobalTexturePageSwapOperation::GetGlobalTextureCopySubAllocation()
	{
		return mGlobalTextureCopySubAllocation;
	}

	const D3D12::TextureCopyBufferSubAllocation& GlobalTexturePageSwapOperation::GetGlobalTextureCopySubAllocation() const
	{
		return mGlobalTextureCopySubAllocation;
	}

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

	std::uint8_t GlobalTexturePageSwapOperation::GetGlobalTextureDescriptionBufferIndex() const
	{
		return mInitInfo.GlobalTextureDescriptionBufferIndex;
	}

	CD3DX12_BOX GlobalTexturePageSwapOperation::GetGlobalTextureCopyRegionBox() const
	{
		const std::int32_t originXCoordinate = (static_cast<std::int32_t>(mInitInfo.GlobalTextureXPageCoordinates) * mInitInfo.PageDimensions);
		const std::int32_t originYCoordinate = (static_cast<std::int32_t>(mInitInfo.GlobalTextureYPageCoordinates) * mInitInfo.PageDimensions);

		return CD3DX12_BOX{
			originXCoordinate,
			originYCoordinate,
			static_cast<std::int32_t>(originXCoordinate + mInitInfo.PageDimensions),
			static_cast<std::int32_t>(originYCoordinate + mInitInfo.PageDimensions)
		};
	}
}