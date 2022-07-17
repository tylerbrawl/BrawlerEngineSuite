module;
#include <memory>
#include <vector>
#include <cassert>
#include <span>
#include <atomic>
#include <ranges>
#include <filesystem>
#include <fstream>
#include <format>
#include <bit>
#include <DxDef.h>
#include <DirectXTex.h>

export module Brawler.VirtualTextureCPUDataStore;
import :VirtualTextureCPUPageStore;
import Util.Math;
import Util.Engine;
import Util.Coroutine;
import Brawler.VirtualTexturePage;
import Brawler.VirtualTextureConstants;
import Brawler.D3D12.FrameGraphBuilding;
import Brawler.NZStringView;
import Brawler.JobSystem;
import Brawler.FilePathHash;
import Util.ModelExport;
import Brawler.LaunchParams;
import Brawler.SerializedVirtualTextureDescriptions;
import Brawler.MappedFileView;
import Brawler.FileAccessMode;

namespace Brawler
{
	template <DXGI_FORMAT TextureFormat, VirtualTexturePageFilterMode FilterMode>
	class VirtualTextureCPUDataStore
	{
	public:
		VirtualTextureCPUDataStore() = default;

		VirtualTextureCPUDataStore(const VirtualTextureCPUDataStore& rhs) = delete;
		VirtualTextureCPUDataStore& operator=(const VirtualTextureCPUDataStore& rhs) = delete;

		VirtualTextureCPUDataStore(VirtualTextureCPUDataStore&& rhs) noexcept = default;
		VirtualTextureCPUDataStore& operator=(VirtualTextureCPUDataStore&& rhs) noexcept = default;

		void CreateCPUPageStores(const std::span<VirtualTexturePage> pageSpan, D3D12::FrameGraphBuilder& frameGraphBuilder);
		FilePathHash SerializeVirtualTexture(const Brawler::NZWStringView srcTextureName, const std::size_t srcTextureDimensions) const;

	private:
		std::vector<VirtualTextureCPUPageStore<TextureFormat, FilterMode>> mPageStoreArr;
	};
}

// ----------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <DXGI_FORMAT TextureFormat, VirtualTexturePageFilterMode FilterMode>
	consteval Brawler::D3D12_RESOURCE_DESC CreateVirtualTexturePageResourceDescription()
	{
		return Brawler::D3D12_RESOURCE_DESC{
			.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			.Alignment = 0,
			.Width = VirtualTextures::GetTotalPageDimensions<FilterMode>().x,
			.Height = VirtualTextures::GetTotalPageDimensions<FilterMode>().y,
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = TextureFormat,
			.SampleDesc{
				.Count = 1,
				.Quality = 0
			},
			.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN,
			.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE,
			.SamplerFeedbackMipRegion{}
		};
	}
}

namespace Brawler
{
	template <DXGI_FORMAT TextureFormat, VirtualTexturePageFilterMode FilterMode>
	void VirtualTextureCPUDataStore<TextureFormat, FilterMode>::CreateCPUPageStores(const std::span<VirtualTexturePage> pageSpan, D3D12::FrameGraphBuilder& frameGraphBuilder)
	{
		mPageStoreArr.clear();
		mPageStoreArr.reserve(pageSpan.size());

		D3D12::RenderPassBundle pageDataReadBackPassBundle{};

		for (auto& page : pageSpan)
		{
			mPageStoreArr.emplace_back(page);
			mPageStoreArr.back().CreatePageDataReadBackRenderPass(page, pageDataReadBackPassBundle);
		}

		frameGraphBuilder.AddRenderPassBundle(std::move(pageDataReadBackPassBundle));
	}

	template <DXGI_FORMAT TextureFormat, VirtualTexturePageFilterMode FilterMode>
	FilePathHash VirtualTextureCPUDataStore<TextureFormat, FilterMode>::SerializeVirtualTexture(const Brawler::NZWStringView srcTextureName, const std::size_t srcTextureDimensions) const
	{
		assert(Util::Math::IsPowerOfTwo(srcTextureDimensions));
		
		// Write out the texture data concurrently.
		std::vector<std::unique_ptr<TextureDataWriteInfo>> dataWriteInfoPtrArr{};
		dataWriteInfoPtrArr.resize(mPageStoreArr.size());

		std::atomic<std::uint64_t> activeJobsCounter{ mPageStoreArr.size() };

		Brawler::JobGroup pageDataWriteGroup{};
		pageDataWriteGroup.Reserve(mPageStoreArr.size());

		for (const auto i : std::views::iota(0u, mPageStoreArr.size()))
		{
			const VirtualTextureCPUPageStore<TextureFormat, FilterMode>& currPageStore{ mPageStoreArr[i] };
			std::unique_ptr<TextureDataWriteInfo>& currWriteInfoPtr{ dataWriteInfoPtrArr[i] };

			pageDataWriteGroup.AddJob([srcTextureName, &currPageStore, &currWriteInfoPtr, &activeJobsCounter] ()
			{
				currWriteInfoPtr = currPageStore.CompressVirtualTexturePage();
				activeJobsCounter.fetch_sub(1, std::memory_order::release);
			});
		}

		// Execute these jobs asynchronously. In the meantime, we can prepare some of the file data
		// beforehand.
		pageDataWriteGroup.ExecuteJobsAsync();

		constexpr Brawler::D3D12_RESOURCE_DESC VIRTUAL_TEXTURE_PAGE_RESOURCE_DESCRIPTION{ CreateVirtualTexturePageResourceDescription<TextureFormat, FilterMode>() };
		std::uint64_t copyablePageSizeInBytes = 0;
		
		Util::Engine::GetD3D12Device().GetCopyableFootprints1(
			&VIRTUAL_TEXTURE_PAGE_RESOURCE_DESCRIPTION,
			0,
			1,
			0,
			nullptr,
			nullptr,
			nullptr,
			&copyablePageSizeInBytes
		);

		const Brawler::LaunchParams& launchParams{ Util::ModelExport::GetLaunchParameters() };
		const std::filesystem::path partialOutputTextureDescFilePath{ std::filesystem::path{ L"Textures" } / launchParams.GetModelName() / std::format(L"{}.bvtx", srcTextureName.C_Str()) };

		const std::filesystem::path completeOutputTextureDescFilePath{ launchParams.GetRootOutputDirectory() / partialOutputTextureDescFilePath };

		std::filesystem::create_directories(completeOutputTextureDescFilePath.parent_path());
		std::size_t startingOffsetForPageData = 0;
		std::size_t totalVirtualTextureFileSize = 0;

		// This is an RAII scope for the file stream.
		{
			std::ofstream outputTextureDescFileStream{ completeOutputTextureDescFilePath, std::ios::out | std::ios::binary };

			constexpr CommonVirtualTextureDescriptionHeader COMMON_HEADER{ CreateCommonVirtualTextureDescriptionHeader() };

			const std::size_t logicalMipLevelCount = (std::countr_zero(srcTextureDimensions) + 1);
			assert(srcTextureDimensions <= std::numeric_limits<std::uint32_t>::max());

			const MergedVirtualTextureDescriptionHeader mergedHeader{
				.CommonHeader{COMMON_HEADER},
				.VersionedHeader{
					.CopyableFootprintsPageSizeInBytes = copyablePageSizeInBytes,
					.TextureFormat = TextureFormat,
					.LogicalTextureMip0Dimensions = static_cast<std::uint32_t>(srcTextureDimensions),
					.LogicalMipLevelCount = static_cast<std::uint32_t>(logicalMipLevelCount),
					.PageCount = static_cast<std::uint32_t>(mPageStoreArr.size())
				}
			};

			outputTextureDescFileStream.write(reinterpret_cast<const char*>(&mergedHeader), sizeof(mergedHeader));

			// SerializedVirtualTexturePageDescription is declared within #pragma pack(1), so using a std::vector
			// here will work out fine for alignment (that is, for the sake of serialization).
			std::vector<SerializedVirtualTexturePageDescription> pageDescArr{};
			pageDescArr.reserve(mPageStoreArr.size());

			// The compressed texture page data will start immediately after the list of page descriptions, which
			// itself comes immediately after the MergedVirtualTextureDescriptionHeader. Each page being written
			// to the file has a page description.
			std::size_t currOffsetForPageData = (sizeof(MergedVirtualTextureDescriptionHeader) + (sizeof(SerializedVirtualTexturePageDescription) * mPageStoreArr.size()));
			startingOffsetForPageData = currOffsetForPageData;

			// At this point, we don't have anything left to do until the texture page write jobs have
			// finished, so we need to wait.
			while (activeJobsCounter.load(std::memory_order::acquire) > 0)
				Util::Coroutine::TryExecuteJob();

			for (const auto& dataWriteInfoPtr : dataWriteInfoPtrArr)
			{
				pageDescArr.push_back(SerializedVirtualTexturePageDescription{
					.OffsetFromDescriptionStart = currOffsetForPageData,
					.LogicalMipLevel = dataWriteInfoPtr->StartingLogicalMipLevel,
					.PageCoordinates{ dataWriteInfoPtr->PageCoordinates },
					});

				// Compressed texture data for the next page begins immediately after this page's compressed texture
				// data.
				currOffsetForPageData += dataWriteInfoPtr->CompressedTexturePageByteArr.size();
			}

			const std::span<const SerializedVirtualTexturePageDescription> pageDescSpan{ pageDescArr };
			outputTextureDescFileStream.write(reinterpret_cast<const char*>(pageDescSpan.data()), pageDescSpan.size_bytes());

			// Rather than query for the file's size later, we can just set totalVirtualTextureFileSize to
			// currOffsetForPageData, since it should have this value already.
			totalVirtualTextureFileSize = currOffsetForPageData;
		}

		// Finally, write out all of the texture data in the same order as we wrote out the texture page
		// descriptions. Each virtual texture can produce dozens of pages, and each model might contain
		// dozens of textures. This could result in thousands (literally, from anecdotal experience) of
		// virtual texture pages being created!
		//
		// Perhaps the best idea, then, is to do the following:
		//
		//   - Re-size the file to contain everything which we already wrote in addition to the compressed
		//     texture data.
		//
		//   - Use memory-mapped I/O to write the compressed data of each page individually.
		std::filesystem::resize_file(completeOutputTextureDescFilePath, totalVirtualTextureFileSize);

		MappedFileView<FileAccessMode::READ_WRITE> completeOutputTextureDescView{ completeOutputTextureDescFilePath, MappedFileView<FileAccessMode::READ_WRITE>::ViewParams{
			.FileOffsetInBytes = startingOffsetForPageData,
			.ViewSizeInBytes = (totalVirtualTextureFileSize - startingOffsetForPageData)
		} };
		assert(completeOutputTextureDescView.IsValidView() && "ERROR: A valid MappedFileView could not be created for writing out virtual texture data!");

		std::span<std::byte> mappedFileDataSpan{ completeOutputTextureDescView.GetMappedData() };

		for (const auto& dataWriteInfoPtr : dataWriteInfoPtrArr)
		{
			std::ranges::copy(dataWriteInfoPtr->CompressedTexturePageByteArr, mappedFileDataSpan.begin());
			mappedFileDataSpan = mappedFileDataSpan.subspan(dataWriteInfoPtr->CompressedTexturePageByteArr.size());
		}

		return FilePathHash{ partialOutputTextureDescFilePath.c_str() };
	}
}

export namespace Brawler
{
	template <DXGI_FORMAT TextureFormat>
	using PointFilterVirtualTextureCPUDataStore = VirtualTextureCPUDataStore<TextureFormat, VirtualTexturePageFilterMode::POINT_FILTER>;

	template <DXGI_FORMAT TextureFormat>
	using BilinearFilterVirtualTextureCPUDataStore = VirtualTextureCPUDataStore<TextureFormat, VirtualTexturePageFilterMode::BILINEAR_FILTER>;

	template <DXGI_FORMAT TextureFormat>
	using TrilinearFilterVirtualTextureCPUDataStore = VirtualTextureCPUDataStore<TextureFormat, VirtualTexturePageFilterMode::TRILINEAR_FILTER>;

	template <DXGI_FORMAT TextureFormat>
	using AnisotropicFilterVirtualTextureCPUDataStore = VirtualTextureCPUDataStore<TextureFormat, VirtualTexturePageFilterMode::ANISOTROPIC_8X_FILTER>;
}