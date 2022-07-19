module;
#include <atomic>
#include <memory>
#include <array>
#include <DxDef.h>

export module Brawler.GlobalTextureUploadBuffer;
import Brawler.D3D12.BufferResource;
import Util.D3D12;
import Util.Math;

namespace Brawler
{
	static constexpr std::size_t MAX_PAGES_PER_UPLOAD_BUFFER = 50;

	template <DXGI_FORMAT Format>
	struct UploadFormatInfo
	{
		static_assert(sizeof(Format) != sizeof(Format), "ERROR: An explicit template specialization of UploadFormatInfo was never provided for a given DXGI_FORMAT! (See GlobalTextureUploadBuffer.ixx.)");
	};

	template <>
	struct UploadFormatInfo<DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM_SRGB>
	{
		// BC7_UNORM_SRGB virtual texture pages are padded to support anisotropic 8x filtering.
		// The default page size is 128 x 128, but these pages have sizes of 136 x 136.
		static constexpr std::uint32_t PADDED_PAGE_DIMENSIONS = 136;
	};

	template <DXGI_FORMAT Format>
	consteval std::size_t GetVirtualTexturePageSizeForUploadBuffer()
	{
		const Brawler::D3D12_RESOURCE_DESC pageDesc{
			.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			.Alignment = 0,
			.Width = UploadFormatInfo<Format>::PADDED_PAGE_DIMENSIONS,
			.Height = UploadFormatInfo<Format>::PADDED_PAGE_DIMENSIONS,
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM_SRGB,
			.SampleDesc{
				.Count = 1,
				.Quality = 0
			},
			.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN,
			.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE,
			.SamplerFeedbackMipRegion{}
		};

		const Util::D3D12::CopyableFootprintsParams footprintsParams{
			.ResourceDesc{ pageDesc },
			.FirstSubResource = 0,
			.NumSubResources = 1,
			.BaseOffset = 0
		};

		const Util::D3D12::CopyableFootprintsResults footprintsResults{ Util::D3D12::GetCopyableFootprints(footprintsParams) };
		return footprintsResults.TotalBytes;
	}

	static_assert(MAX_PAGES_PER_UPLOAD_BUFFER > 0);

	template <DXGI_FORMAT Format>
	consteval std::size_t GetRequiredUploadBufferSize()
	{
		const std::size_t pageSizeInBuffer = GetVirtualTexturePageSizeForUploadBuffer<Format>();
		const std::size_t alignedPageSizeInBuffer = Util::Math::AlignToPowerOfTwo(pageSizeInBuffer, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

		return (pageSizeInBuffer + ((MAX_PAGES_PER_UPLOAD_BUFFER - 1) * alignedPageSizeInBuffer));
	}
}

export namespace Brawler
{
	template <DXGI_FORMAT Format>
	class GlobalTextureUploadBuffer
	{
	public:
		GlobalTextureUploadBuffer() = default;

		GlobalTextureUploadBuffer(const GlobalTextureUploadBuffer& rhs) = delete;
		GlobalTextureUploadBuffer& operator=(const GlobalTextureUploadBuffer& rhs) = delete;

		GlobalTextureUploadBuffer(GlobalTextureUploadBuffer&& rhs) noexcept = default;
		GlobalTextureUploadBuffer& operator=(GlobalTextureUploadBuffer&& rhs) noexcept = default;



	private:
		std::unique_ptr<D3D12::BufferResource> mUploadBufferPtr;
		std::atomic<std::uint64_t> mAvailableIndexCounter;

		/// <summary>
		/// We need to prevent writes into the upload buffer until after we know
		/// that the GPU has, in fact, saved the data.
		/// </summary>
		std::atomic<std::uint64_t> mMinimumFrameNumberRequired;
	};
}