module;
#include <span>
#include <vector>
#include <ranges>
#include <DxDef.h>

export module Brawler.GenericPreFrameTextureUpdate;
import Brawler.D3D12.TextureSubResource;
import Brawler.D3D12.TextureCopyRegion;

export namespace Brawler
{
	class GenericPreFrameTextureUpdate
	{
	public:
		GenericPreFrameTextureUpdate() = default;
		explicit GenericPreFrameTextureUpdate(D3D12::TextureSubResource textureSubResource);
		GenericPreFrameTextureUpdate(D3D12::TextureSubResource textureSubResource, CD3DX12_BOX copyRegionBox);

		GenericPreFrameTextureUpdate(const GenericPreFrameTextureUpdate& rhs) = delete;
		GenericPreFrameTextureUpdate& operator=(const GenericPreFrameTextureUpdate& rhs) = delete;

		GenericPreFrameTextureUpdate(GenericPreFrameTextureUpdate&& rhs) noexcept = default;
		GenericPreFrameTextureUpdate& operator=(GenericPreFrameTextureUpdate&& rhs) noexcept = default;

		template <typename T>
		void SetInputData(const std::span<const T> dataSpan);

		D3D12::TextureSubResource& GetDestinationTextureSubResource();
		const D3D12::TextureSubResource& GetDestinationTextureSubResource() const;

		D3D12::TextureCopyRegion GetTextureCopyRegion() const;

		std::span<const std::byte> GetDataByteSpan() const;
		std::vector<std::byte> ExtractDataByteArray();

		std::size_t GetUpdateRegionSize() const;

	private:
		D3D12::TextureSubResource mDestSubResource;
		CD3DX12_BOX mCopyRegionBox;
		std::vector<std::byte> mDataArr;
	};
}

// --------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename T>
	void GenericPreFrameTextureUpdate::SetInputData(const std::span<const T> dataSpan)
	{
		mDataArr.resize(dataSpan.size_bytes());

		const std::span<const std::byte> srcDataByteSpan{ std::as_bytes(dataSpan) };
		std::ranges::copy(srcDataByteSpan, mDataArr.begin());
	}
}