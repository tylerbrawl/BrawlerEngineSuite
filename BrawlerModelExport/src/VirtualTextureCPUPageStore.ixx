module;
#include <memory>
#include <fstream>
#include <filesystem>
#include <optional>
#include <format>
#include <span>
#include <DxDef.h>
#include <DirectXMath/DirectXMath.h>

export module Brawler.VirtualTextureCPUDataStore:VirtualTextureCPUPageStore;
import Brawler.VirtualTexturePage;
import Brawler.VirtualTextureConstants;
import Brawler.VirtualTexturePageFilterMode;
import Brawler.D3D12.FrameGraphBuilding;
import Brawler.D3D12.BufferResource;
import Brawler.D3D12.BufferResourceInitializationInfo;
import Util.D3D12;
import Brawler.D3D12.Texture2D;
import Brawler.D3D12.BufferSubAllocationReservationHandle;
import Brawler.D3D12.TextureCopyBufferSubAllocation;
import Brawler.FilePathHash;
import Brawler.NZStringView;
import Util.ModelExport;
import Brawler.LaunchParams;
import Brawler.MappedFileView;
import Brawler.FileAccessMode;

export namespace Brawler
{
	struct TextureDataWriteInfo
	{
		FilePathHash TextureDataPathHash;
		std::uint32_t StartingLogicalMipLevel;
		DirectX::XMUINT2 PageCoordinates;
	};
}

export namespace Brawler
{
	template <DXGI_FORMAT TextureFormat, VirtualTexturePageFilterMode FilterMode>
	class VirtualTextureCPUPageStore
	{
	public:
		VirtualTextureCPUPageStore() = default;
		explicit VirtualTextureCPUPageStore(const VirtualTexturePage& page);

		VirtualTextureCPUPageStore(const VirtualTextureCPUPageStore& rhs) = delete;
		VirtualTextureCPUPageStore& operator=(const VirtualTextureCPUPageStore& rhs) = delete;

		VirtualTextureCPUPageStore(VirtualTextureCPUPageStore&& rhs) noexcept = default;
		VirtualTextureCPUPageStore& operator=(VirtualTextureCPUPageStore&& rhs) noexcept = default;

		void CreatePageDataReadBackRenderPass(VirtualTexturePage& page, D3D12::RenderPassBundle& renderPassBundle);
		std::unique_ptr<TextureDataWriteInfo> SerializeVirtualTexturePage(const Brawler::NZWStringView srcTextureName) const;

	private:
		auto GetRenderPassForUncompressedTextureReadBack(const D3D12::Texture2DSubResource pageTextureSubResource);
		auto GetRenderPassForCompressedTextureReadBack(D3D12::BufferSubAllocationReservationHandle&& hCompressedDataReservation);

	private:
		std::unique_ptr<D3D12::BufferResource> mReadbackBufferPtr;
		std::uint32_t mStartingLogicalMipLevel;
		DirectX::XMUINT2 mPageCoordinates;
	};
}

// -----------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <DXGI_FORMAT TextureFormat, VirtualTexturePageFilterMode FilterMode>
	consteval std::size_t GetVirtualTexturePageSizeInBytes()
	{
		std::size_t numBytesPerTexel = (Util::D3D12::GetBitsPerPixelForFormat(TextureFormat) / 8);
		DirectX::XMUINT2 totalPageDimensions = VirtualTextures::GetTotalPageDimensions<FilterMode>();

		return (static_cast<std::size_t>(totalPageDimensions.x) * static_cast<std::size_t>(totalPageDimensions.y)) * numBytesPerTexel;
	}
}

namespace Brawler
{
	template <DXGI_FORMAT TextureFormat, VirtualTexturePageFilterMode FilterMode>
	VirtualTextureCPUPageStore<TextureFormat, FilterMode>::VirtualTextureCPUPageStore(const VirtualTexturePage& page) :
		mReadbackBufferPtr(nullptr),
		mStartingLogicalMipLevel(page.GetStartingLogicalMipLevel()),
		mPageCoordinates(page.GetPageCoordinates())
	{
		// Create the persistent read-back buffer for this page.
		constexpr D3D12::BufferResourceInitializationInfo BUFFER_INITIALIZATION_INFO{
			.SizeInBytes = GetVirtualTexturePageSizeInBytes<TextureFormat, FilterMode>(),
			.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK
		};

		mReadbackBufferPtr = std::make_unique<D3D12::BufferResource>(BUFFER_INITIALIZATION_INFO);
	}

	template <DXGI_FORMAT TextureFormat, VirtualTexturePageFilterMode FilterMode>
	void VirtualTextureCPUPageStore<TextureFormat, FilterMode>::CreatePageDataReadBackRenderPass(VirtualTexturePage& page, D3D12::RenderPassBundle& renderPassBundle)
	{
		assert(mReadbackBufferPtr != nullptr);
		
		// If the VirtualTexturePage instance has compressed data, then we will use that to write into the read-back
		// buffer. Otherwise, we will read from the Texture2DSubResource contained within the VirtualTexturePage
		// instance, which has the uncompressed data.
		if (page.HasCompressedDataBufferReservation()) [[likely]]
			renderPassBundle.AddRenderPass(GetRenderPassForCompressedTextureReadBack(frameGraphBuilder, page.RevokeCompressedDataBufferReservation()));
		else [[unlikely]]
			renderPassBundle.AddRenderPass(GetRenderPassForUncompressedTextureReadBack(frameGraphBuilder, page.GetTexture2DSubResource()));
	}

	template <DXGI_FORMAT TextureFormat, VirtualTexturePageFilterMode FilterMode>
	std::unique_ptr<TextureDataWriteInfo> VirtualTextureCPUPageStore<TextureFormat, FilterMode>::SerializeVirtualTexturePage(const Brawler::NZWStringView srcTextureName) const
	{
		const Brawler::LaunchParams& launchParams{ Util::ModelExport::GetLaunchParameters() };
		const std::filesystem::path partialOutputTexturePageFilePath{ std::filesystem::path{ L"Textures" } / launchParams.GetModelName() / std::format(
			L"{}_VTPage_Mip{}_{}_{}.bvtp",
			srcTextureName.C_Str(),
			mStartingLogicalMipLevel,
			mPageCoordinates.x,
			mPageCoordinates.y
		) };

		std::unique_ptr<TextureDataWriteInfo> dataWriteInfo{ std::make_unique<TextureDataWriteInfo>(TextureDataWriteInfo{
			.TextureDataPathHash{ std::wstring_view{ partialOutputTexturePageFilePath.c_str() } },
			.StartingLogicalMipLevel = mStartingLogicalMipLevel,
			.PageCoordinates{ mPageCoordinates }
		}) };

		const std::filesystem::path completeOutputTexturePageFilePath{ launchParams.GetRootOutputDirectory() / partialOutputTexturePageFilePath };
		std::filesystem::create_directories(completeOutputTexturePageFilePath.parent_path());

		// Create the file if it does not already exist. If it does, then erase its contents.
		{
			std::ofstream outputTexturePageFileStream{ completeOutputTexturePageFilePath, std::ios::out | std::ios::binary };
		}

		// Re-size the created file to hold the contents of the page.
		constexpr std::size_t NUM_BYTES_PER_PAGE = GetVirtualTexturePageSizeInBytes<TextureFormat, FilterMode>();
		std::filesystem::resize_file(completeOutputTexturePageFilePath, NUM_BYTES_PER_PAGE);

		// Create a MappedFileView to the file which we just created/cleared.
		MappedFileView<FileAccessMode::READ_WRITE> outputTexturePageFileView{ completeOutputTexturePageFilePath };
		
		std::optional<D3D12::StructuredBufferSubAllocation<std::byte>> readbackDataSubAllocation{ mReadbackBufferPtr->CreateBufferSubAllocation<D3D12::StructuredBufferSubAllocation<std::byte>>(NUM_BYTES_PER_PAGE) };
		assert(readbackDataSubAllocation.has_value());

		// Write the texture data directly from the read-back heap into the file on the filesystem.
		const std::span<std::byte> mappedPageDataSpan{ outputTexturePageFileView.GetMappedData() };
		readbackDataSubAllocation.ReadStructuredBufferData(0, mappedPageDataSpan);

		return dataWriteInfo;
	}

	template <DXGI_FORMAT TextureFormat, VirtualTexturePageFilterMode FilterMode>
	auto VirtualTextureCPUPageStore<TextureFormat, FilterMode>::GetRenderPassForUncompressedTextureReadBack(const D3D12::Texture2DSubResource pageTextureSubResource)
	{
		struct UncompressedTextureCopyPassInfo
		{
			D3D12::Texture2DSubResource SrcTextureSubResource;
			D3D12::TextureCopyBufferSnapshot DestBufferSnapshot;
		};

		std::optional<D3D12::TextureCopyBufferSubAllocation> destBufferSubAllocation{ mReadbackBufferPtr->CreateBufferSubAllocation<D3D12::TextureCopyBufferSubAllocation>(pageTextureSubResource) };
		assert(destBufferSubAllocation.has_value());

		D3D12::RenderPass<D3D12::GPUCommandQueueType::DIRECT, UncompressedTextureCopyPassInfo> uncompressedCopyPass{};
		uncompressedCopyPass.SetRenderPassName("Virtual Texture CPU Transfer - Uncompressed Texture Copy");

		uncompressedCopyPass.AddResourceDependency(*destBufferSubAllocation, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
		uncompressedCopyPass.AddResourceDependency(pageTextureSubResource, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE);

		uncompressedCopyPass.SetInputData(UncompressedTextureCopyPassInfo{
			.SrcTextureSubResource{pageTextureSubResource},
			.DestBufferSnapshot{*destBufferSubAllocation}
		});

		uncompressedCopyPass.SetRenderPassCommands([] (D3D12::DirectContext& context, const UncompressedTextureCopyPassInfo& passInfo)
		{
			context.CopyTextureToBuffer(passInfo.DestBufferSnapshot, passInfo.SrcTextureSubResource);
		});

		return uncompressedCopyPass;
	}

	template <DXGI_FORMAT TextureFormat, VirtualTexturePageFilterMode FilterMode>
	auto VirtualTextureCPUPageStore<TextureFormat, FilterMode>::GetRenderPassForCompressedTextureReadBack(D3D12::BufferSubAllocationReservationHandle&& hCompressedDataReservation)
	{
		assert(hCompressedDataReservation.IsReservationValid());

		constexpr std::size_t NUM_BYTES_PER_PAGE = GetVirtualTexturePageSizeInBytes<TextureFormat, FilterMode>();
		assert(hCompressedDataReservation->GetReservationSize() == NUM_BYTES_PER_PAGE);

		D3D12::StructuredBufferSubAllocation<std::byte> compressedDataSubAllocation{ NUM_BYTES_PER_PAGE };
		assert(compressedDataSubAllocation.IsReservationCompatible(hCompressedDataReservation));
		compressedDataSubAllocation.AssignReservation(std::move(hCompressedDataReservation));

		std::optional<D3D12::StructuredBufferSubAllocation<std::byte>> destBufferSubAllocation{ mReadbackBufferPtr->CreateBufferSubAllocation<D3D12::StructuredBufferSubAllocation<std::byte>>(NUM_BYTES_PER_PAGE) };
		assert(destBufferSubAllocation.has_value());

		struct CompressedTextureCopyPassInfo
		{
			D3D12::StructuredBufferSnapshot<std::byte> SrcBufferSnapshot;
			D3D12::StructuredBufferSnapshot<std::byte> DestBufferSnapshot;
		};

		D3D12::RenderPass<D3D12::GPUCommandQueueType::DIRECT, CompressedTextureCopyPassInfo> compressedCopyPass{};
		compressedCopyPass.SetRenderPassName("Virtual Texture CPU Transfer - Compressed Texture Copy");

		compressedCopyPass.AddResourceDependency(*destBufferSubAllocation, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
		compressedCopyPass.AddResourceDependency(compressedDataSubAllocation, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE);

		compressedCopyPass.SetInputData(CompressedTextureCopyPassInfo{
			.SrcBufferSnapshot{compressedDataSubAllocation},
			.DestBufferSnapshot{*destBufferSubAllocation}
		});

		compressedCopyPass.SetRenderPassCommands([] (D3D12::DirectContext& context, const CompressedTextureCopyPassInfo& passInfo)
		{
			context.CopyBufferToBuffer(passInfo.DestBufferSnapshot, passInfo.SrcBufferSnapshot);
		});

		return compressedCopyPass;
	}
}