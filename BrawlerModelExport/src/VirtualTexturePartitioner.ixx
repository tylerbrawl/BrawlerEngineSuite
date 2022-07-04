module;
#include <vector>
#include <cassert>
#include <ranges>
#include <optional>
#include <bit>
#include <memory>
#include <DxDef.h>
#include <DirectXMath/DirectXMath.h>

export module Brawler.VirtualTexturePartitioner;
import Brawler.VirtualTexturePageFilterMode;
import Brawler.VirtualTextureConstants;
import Brawler.D3D12.Texture2D;
import Brawler.VirtualTexturePage;
import Brawler.D3D12.FrameGraphBuilding;
import Util.D3D12;
import Util.General;
import Util.Math;
import Brawler.D3D12.BufferResource;
import Brawler.D3D12.BufferResourceInitializationInfo;
import Brawler.D3D12.ConstantBufferSubAllocation;
import Brawler.D3D12.StructuredBufferSubAllocation;
import Brawler.D3D12.BufferCopyRegion;
import Brawler.D3D12.DescriptorTableBuilder;
import Brawler.D3D12.Texture2DBuilders;
import Brawler.D3D12.PipelineEnums;
import Brawler.D3D12.GPUResourceBinding;

namespace Brawler
{
	struct MipLevelInfo
	{
		std::uint32_t InputMipLevel;
		std::uint32_t MipLevelLogicalSize;
		DirectX::XMUINT2 __Pad0;
	};
}

namespace Brawler
{
	template <DXGI_FORMAT TextureFormat, VirtualTexturePageFilterMode FilterMode>
	class VirtualTexturePartitioner
	{
	public:
		VirtualTexturePartitioner() = default;
		explicit VirtualTexturePartitioner(D3D12::Texture2D& srcTexture);

		VirtualTexturePartitioner(const VirtualTexturePartitioner& rhs) = delete;
		VirtualTexturePartitioner& operator=(const VirtualTexturePartitioner& rhs) = delete;

		VirtualTexturePartitioner(VirtualTexturePartitioner&& rhs) noexcept = default;
		VirtualTexturePartitioner& operator=(VirtualTexturePartitioner&& rhs) noexcept = default;

		void PartitionVirtualTexture(D3D12::FrameGraphBuilder& frameGraphBuilder);
		std::vector<VirtualTexturePage> ExtractVirtualTexturePages();

	private:
		void InitializeConstantBufferData(D3D12::FrameGraphBuilder& frameGraphBuilder);
		void PartitionLargeMipMap(D3D12::FrameGraphBuilder& frameGraphBuilder, D3D12::Texture2DSubResource mipMapSubResource);
		void MergeSmallMipMaps(D3D12::FrameGraphBuilder& frameGraphBuilder, const std::uint32_t firstMipLevelToMerge);

	private:
		D3D12::Texture2D* mSrcTexturePtr;
		std::vector<VirtualTexturePage> mPageArr;
		std::vector<D3D12::ConstantBufferSubAllocation<MipLevelInfo>> mCBSubAllocationArr;
		std::shared_ptr<D3D12::DescriptorTableBuilder> mSrcTextureTableBuilderPtr;
	};
}

// ---------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <DXGI_FORMAT Format>
	consteval bool IsSRGB()
	{
		switch (Format)
		{
		case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: [[fallthrough]];
		case DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM_SRGB: [[fallthrough]];
		case DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM_SRGB: [[fallthrough]];
		case DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM_SRGB: [[fallthrough]];
		case DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: [[fallthrough]];
		case DXGI_FORMAT::DXGI_FORMAT_B8G8R8X8_UNORM_SRGB: [[fallthrough]];
		case DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM_SRGB:
			return true;

		default:
			return false;
		}
	}

	template <DXGI_FORMAT Format>
	consteval DXGI_FORMAT GetUAVFormatForSRGBFormat()
	{
		switch (Format)
		{
		case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;

		case DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM_SRGB:
			return DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM;

		case DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM_SRGB:
			return DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM;

		case DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM_SRGB:
			return DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM;

		case DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			return DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM;

		case DXGI_FORMAT::DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
			return DXGI_FORMAT::DXGI_FORMAT_B8G8R8X8_UNORM;

		case DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM_SRGB:
			return DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM_SRGB;

		default:
			assert(false);

			return DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		}
	}

	template <DXGI_FORMAT TextureFormat, VirtualTexturePageFilterMode FilterMode>
	consteval D3D12::Texture2DBuilder CreateVirtualTexturePageBuilder()
	{
		D3D12::Texture2DBuilder textureBuilder{};
		textureBuilder.AllowUnorderedAccessViews();
		textureBuilder.SetInitialResourceState(D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		const DirectX::XMUINT2 totalPageDimensions{ VirtualTextures::GetTotalPageDimensions<FilterMode>() };
		textureBuilder.SetTextureDimensions(totalPageDimensions.x, totalPageDimensions.y);

		if (IsSRGB<TextureFormat>())
			textureBuilder.SetTextureFormat(GetUAVFormatForSRGBFormat<TextureFormat>());
		else
			textureBuilder.SetTextureFormat(TextureFormat);

		return textureBuilder;
	}

	template <DXGI_FORMAT TextureFormat, VirtualTexturePageFilterMode FilterMode>
	consteval Brawler::PSOs::PSOID GetLargeMipLevelPartitionPSOIdentifier()
	{
		switch (FilterMode)
		{
		case VirtualTexturePageFilterMode::POINT_FILTER:
			return (IsSRGB<TextureFormat>() ? Brawler::PSOs::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_POINT_SRGB : Brawler::PSOs::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_POINT);

		case VirtualTexturePageFilterMode::BILINEAR_FILTER:
			return (IsSRGB<TextureFormat>() ? Brawler::PSOs::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_BILINEAR_SRGB : Brawler::PSOs::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_BILINEAR);

		case VirtualTexturePageFilterMode::TRILINEAR_FILTER:
			return (IsSRGB<TextureFormat>() ? Brawler::PSOs::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_TRILINEAR_SRGB : Brawler::PSOs::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_TRILINEAR);

		case VirtualTexturePageFilterMode::ANISOTROPIC_8X_FILTER:
			return (IsSRGB<TextureFormat>() ? Brawler::PSOs::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_ANISOTROPIC_SRGB : Brawler::PSOs::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_ANISOTROPIC);

		default:
		{
			assert(false && "ERROR: A VirtualTexturePageFilterMode value was never assigned any PSOID value(s) for large mip level partitioning!");

			return Brawler::PSOs::PSOID::COUNT_OR_ERROR;
		}
		}
	}

	template <DXGI_FORMAT TextureFormat, VirtualTexturePageFilterMode FilterMode>
	consteval Brawler::PSOs::PSOID GetSmallMipLevelMergePSOIdentifier()
	{
		switch (FilterMode)
		{
		case VirtualTexturePageFilterMode::POINT_FILTER:
			return (IsSRGB<TextureFormat>() ? Brawler::PSOs::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_POINT_SRGB : Brawler::PSOs::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_POINT);

		case VirtualTexturePageFilterMode::BILINEAR_FILTER:
			return (IsSRGB<TextureFormat>() ? Brawler::PSOs::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_BILINEAR_SRGB : Brawler::PSOs::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_BILINEAR);

		case VirtualTexturePageFilterMode::TRILINEAR_FILTER:
			return (IsSRGB<TextureFormat>() ? Brawler::PSOs::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_TRILINEAR_SRGB : Brawler::PSOs::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_TRILINEAR);

		case VirtualTexturePageFilterMode::ANISOTROPIC_8X_FILTER:
			return (IsSRGB<TextureFormat>() ? Brawler::PSOs::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_ANISOTROPIC_SRGB : Brawler::PSOs::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_ANISOTROPIC);

		default:
		{
			assert(false && "ERROR: A VirtualTexturePageFilterMode value was never assigned any PSOID value(s) for small mip level merging!");

			return Brawler::PSOs::PSOID::COUNT_OR_ERROR;
		}
		}
	}
}

namespace Brawler
{
	template <DXGI_FORMAT TextureFormat, VirtualTexturePageFilterMode FilterMode>
	VirtualTexturePartitioner<TextureFormat, FilterMode>::VirtualTexturePartitioner(D3D12::Texture2D& srcTexture) :
		mSrcTexturePtr(&srcTexture),
		mPageArr(),
		mCBSubAllocationArr(),
		mSrcTextureTableBuilderPtr(nullptr)
	{
		if constexpr (Util::General::IsDebugModeEnabled())
		{
			const Brawler::D3D12_RESOURCE_DESC& srcTextureDesc{ srcTexture.GetResourceDescription() };
			assert(Util::D3D12::IsSRVRTVDSVResourceCastLegal(srcTextureDesc.Format, TextureFormat) && "ERROR: An invalid DXGI_FORMAT value was provided when instantiating a VirtualTexturePartitioner instance!");
			assert(srcTextureDesc.Width == srcTextureDesc.Height && Util::Math::IsPowerOfTwo(srcTextureDesc.Width) && "ERROR: Virtual textures must be square, and they must have dimensions which are powers of two!");
		}

		// There are a number of reasons as to why the D3D12::DescriptorTableBuilder instance which builds
		// the SRV descriptor table for the source texture is wrapped around a std::shared_ptr:
		//
		//   - There appears to be a race condition with the D3D12 Debug Layer where creating descriptors within
		//     a descriptor heap shortly after the ID3D12DescriptorHeap instance is created causes a crash. As
		//     such, we need to create the D3D12::DescriptorTableBuilder instance (and thus its corresponding
		//     ID3D12DescriptorHeap instance) as soon as possible. Hey, it's not my fault this time, okay?
		//
		//   - Using a std::shared_ptr will allow for automatic lifetime management as the D3D12::DescriptorTableBuilder
		//     instance is used across many different D3D12::RenderPass instances. The D3D12::DescriptorTableBuilder
		//     class is thread safe, so sharing it amongst many recording render passes is safe.
		//
		//   - Since the lifetime of the D3D12::DescriptorTableBuilder instance is now managed by the D3D12::RenderPass
		//     instances, we can safely destroy this VirtualTexturePartitioner instance before any commands are ever
		//     recorded into an ID3D12GraphicsCommandList.
		mSrcTextureTableBuilderPtr = std::make_shared<D3D12::DescriptorTableBuilder>(1);
		mSrcTextureTableBuilderPtr->CreateShaderResourceView(0, mSrcTexturePtr->CreateShaderResourceView<TextureFormat>());
	}

	template <DXGI_FORMAT TextureFormat, VirtualTexturePageFilterMode FilterMode>
	void VirtualTexturePartitioner<TextureFormat, FilterMode>::PartitionVirtualTexture(D3D12::FrameGraphBuilder& frameGraphBuilder)
	{
		InitializeConstantBufferData(frameGraphBuilder);

		// After that function is called, mCBSubAllocationArr contains one D3D12::ConstantBufferSubAllocation
		// instance for every mip level which needs to be partitioned, rather than merged. Thus, we do not
		// need to calculate this again.
		for (const auto i : std::views::iota(0u, mCBSubAllocationArr.size()))
			PartitionLargeMipMap(frameGraphBuilder, mSrcTexturePtr->GetSubResource(i));

		// Merge the remaining mip levels into a single page.
		MergeSmallMipMaps(frameGraphBuilder, static_cast<std::uint32_t>(mCBSubAllocationArr.size()));
	}

	template <DXGI_FORMAT TextureFormat, VirtualTexturePageFilterMode FilterMode>
	std::vector<VirtualTexturePage> VirtualTexturePartitioner<TextureFormat, FilterMode>::ExtractVirtualTexturePages()
	{
		return std::move(mPageArr);
	}

	template <DXGI_FORMAT TextureFormat, VirtualTexturePageFilterMode FilterMode>
	void VirtualTexturePartitioner<TextureFormat, FilterMode>::InitializeConstantBufferData(D3D12::FrameGraphBuilder& frameGraphBuilder)
	{
		static constexpr std::size_t PADDED_MIP_LEVEL_INFO_STRUCTURE_SIZE = Util::Math::AlignToPowerOfTwo(sizeof(MipLevelInfo), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
		std::size_t numLargeMipLevels = 0;

		{
			std::size_t currWidth = mSrcTexturePtr->GetResourceDescription().Width;

			while (currWidth >= VirtualTextures::USEFUL_VIRTUAL_TEXTURE_PAGE_DIMENSIONS.x)
			{
				++numLargeMipLevels;
				currWidth >>= 1;
			}
		}

		// Exit early if all of the mip levels in the source texture are smaller than the useful page dimensions.
		if (numLargeMipLevels == 0) [[unlikely]]
			return;

		// Create an upload buffer which will transfer the constant buffer data to a separate buffer in a DEFAULT
		// heap. Since we are not using the upload buffer directly as the source for constant buffer data, we do not
		// need to pad out its size.
		D3D12::BufferResource& uploadBuffer{ frameGraphBuilder.CreateTransientResource<D3D12::BufferResource>(D3D12::BufferResourceInitializationInfo{
			.SizeInBytes = (sizeof(MipLevelInfo) * numLargeMipLevels),
			.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD
		}) };

		// Create a StructuredBufferSubAllocation to hold the data for each mip level to be transferred.
		std::optional<D3D12::StructuredBufferSubAllocation<MipLevelInfo>> uploadDataSubAllocation{ uploadBuffer.CreateBufferSubAllocation<D3D12::StructuredBufferSubAllocation<MipLevelInfo>>(numLargeMipLevels) };
		assert(uploadDataSubAllocation.has_value());

		// Create a buffer in a DEFAULT heap which will contain the constant buffer data. We expect the typical access
		// pattern for the data to be "write-once, read-many-times;" this is why we do not read the constant buffer data
		// directly from the upload buffer when executing the shader.
		//
		// We need to pad the size of this buffer to account for the required alignment of constant buffer data within
		// a buffer.
		D3D12::BufferResource& defaultBuffer{ frameGraphBuilder.CreateTransientResource<D3D12::BufferResource>(D3D12::BufferResourceInitializationInfo{
			.SizeInBytes = (PADDED_MIP_LEVEL_INFO_STRUCTURE_SIZE * numLargeMipLevels),
			.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT
		}) };

		// Create a separate ConstantBufferSubAllocation for each element.
		mCBSubAllocationArr.reserve(numLargeMipLevels);

		for (const auto i : std::views::iota(0u, numLargeMipLevels))
		{
			std::optional<D3D12::ConstantBufferSubAllocation<MipLevelInfo>> currSubAllocation{ defaultBuffer.CreateBufferSubAllocation<D3D12::ConstantBufferSubAllocation<MipLevelInfo>>() };
			assert(currSubAllocation.has_value());

			mCBSubAllocationArr.push_back(std::move(*currSubAllocation));
		}

		struct ConstantBufferDataCopyInfo
		{
			D3D12::StructuredBufferSubAllocation<MipLevelInfo> SrcDataSubAllocation;
			std::vector<D3D12::ConstantBufferSnapshot<MipLevelInfo>> DestBufferSnapshotArr;
			std::size_t MaxLogicalMipLevelDimensions;
		};

		// Create a RenderPassBundle which contains the RenderPass which initializes the constant buffer data.
		D3D12::RenderPass<D3D12::GPUCommandQueueType::DIRECT, ConstantBufferDataCopyInfo> cbDataCopyPass{};
		cbDataCopyPass.SetRenderPassName("Virtual Texture Partitioning - Large Mip Level Data Copy");

		ConstantBufferDataCopyInfo dataCopyInfo{
			.SrcDataSubAllocation{std::move(*uploadDataSubAllocation)},
			.DestBufferSnapshotArr{},
			.MaxLogicalMipLevelDimensions{ mSrcTexturePtr->GetResourceDescription().Width }
		};

		dataCopyInfo.DestBufferSnapshotArr.reserve(numLargeMipLevels);

		for (const auto& destBufferSubAllocation : mCBSubAllocationArr)
			dataCopyInfo.DestBufferSnapshotArr.emplace_back(destBufferSubAllocation);

		// We can/should only add the resource dependency for one of the ConstantBufferSubAllocation instances.
		cbDataCopyPass.AddResourceDependency(mCBSubAllocationArr[0], D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
		cbDataCopyPass.AddResourceDependency(dataCopyInfo.SrcDataSubAllocation, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE);

		cbDataCopyPass.SetInputData(std::move(dataCopyInfo));

		cbDataCopyPass.SetRenderPassCommands([] (D3D12::DirectContext& context, const ConstantBufferDataCopyInfo& copyInfo)
		{
			const std::size_t numLargeMipLevels = copyInfo.DestBufferSnapshotArr.size();

			// Create the data which we will submit to the GPU. Rather than uploading each MipLevelInfo element one at
			// a time to the GPU, we first initialize all of the instances on the CPU and then upload all elements at
			// once. This reduces the amount of time spent sending data across the PCI-e lane.
			std::vector<MipLevelInfo> mipLevelInfoArr{};
			mipLevelInfoArr.resize(numLargeMipLevels);

			std::size_t currMipLevel = 0;
			for (auto& mipLevelInfo : mipLevelInfoArr)
			{
				mipLevelInfo.InputMipLevel = static_cast<std::uint32_t>(currMipLevel);
				mipLevelInfo.MipLevelLogicalSize = static_cast<std::uint32_t>(copyInfo.MaxLogicalMipLevelDimensions >> currMipLevel);

				++currMipLevel;
			}

			{
				const std::span<const MipLevelInfo> uploadDataSpan{ mipLevelInfoArr };
				copyInfo.SrcDataSubAllocation.WriteStructuredBufferData(0, uploadDataSpan);
			}

			for (const auto i : std::views::iota(0u, numLargeMipLevels))
			{
				const D3D12::BufferCopyRegion destDataRegion{ copyInfo.DestBufferSnapshotArr[i].GetBufferCopyRegion() };
				const D3D12::BufferCopyRegion srcDataRegion{ copyInfo.SrcDataSubAllocation.GetBufferCopyRegion(D3D12::StructuredBufferElementRange{
					.FirstElement = i,
					.NumElements = 1
				}) };

				context.CopyBufferToBuffer(destDataRegion, srcDataRegion);
			}
		});

		D3D12::RenderPassBundle dataCopyPassBundle{};
		dataCopyPassBundle.AddRenderPass(std::move(cbDataCopyPass));

		frameGraphBuilder.AddRenderPassBundle(std::move(dataCopyPassBundle));
	}

	template <DXGI_FORMAT TextureFormat, VirtualTexturePageFilterMode FilterMode>
	void VirtualTexturePartitioner<TextureFormat, FilterMode>::PartitionLargeMipMap(D3D12::FrameGraphBuilder& frameGraphBuilder, D3D12::Texture2DSubResource mipMapSubResource)
	{
		// The dimensions of the current mip level are (MipLevel0Dimensions) / 2^(CurrentMipLevel). However, since
		// we know that the source texture has power-of-two dimensions, we can optimize this calculation as follows:
		const std::size_t currMipLevelDimensions = (mSrcTexturePtr->GetResourceDescription().Width >> mipMapSubResource.GetSubResourceIndex());
		assert(static_cast<std::uint32_t>(currMipLevelDimensions) >= VirtualTextures::USEFUL_VIRTUAL_TEXTURE_PAGE_DIMENSIONS.x);

		// Since we only partition mip levels which have at least as many texels as the useful dimensions of a page,
		// and since both the useful page dimensions and highest mip level of the source texture are square powers
		// of two, we can easily calculate the number of pages needed as 
		// (CurrMipLevelWidth * CurrMipLevelHeight) / (UsefulPageWidth * UsefulPageHeight). The equations below
		// are optimized versions of this expression.
		static constexpr std::size_t NUM_USEFUL_PAGE_TEXELS = static_cast<std::size_t>(VirtualTextures::USEFUL_VIRTUAL_TEXTURE_PAGE_DIMENSIONS.x) * static_cast<std::size_t>(VirtualTextures::USEFUL_VIRTUAL_TEXTURE_PAGE_DIMENSIONS.y);
		static constexpr std::size_t SIZE_DIVISOR_EXPONENT = std::countr_zero(NUM_USEFUL_PAGE_TEXELS);
		const std::size_t numPagesToCreate = (currMipLevelDimensions << std::countr_zero(currMipLevelDimensions)) >> SIZE_DIVISOR_EXPONENT;

		const std::uint32_t numPagesPerLogicalRow = static_cast<std::uint32_t>(currMipLevelDimensions / VirtualTextures::USEFUL_VIRTUAL_TEXTURE_PAGE_DIMENSIONS.x);

		D3D12::ConstantBufferSubAllocation<MipLevelInfo>& currMipLevelInfoSubAllocation{ mCBSubAllocationArr[mipMapSubResource.GetSubResourceIndex()] };

		D3D12::RenderPassBundle mipLevelPartitionPassBundle{};
		std::size_t numPagesCreated = 0;
		DirectX::XMUINT2 currStartingCoordinates{};
		DirectX::XMUINT2 currPageCoordinates{};

		std::vector<D3D12::Texture2D*> outputPageTexturePtrArr{};
		outputPageTexturePtrArr.reserve(4);

		static constexpr std::size_t MAX_OUTPUT_PAGES_PER_DISPATCH = 4;

		while (numPagesCreated < numPagesToCreate)
		{
			const std::size_t numPagesThisIteration = std::min<std::size_t>(numPagesToCreate - numPagesCreated, MAX_OUTPUT_PAGES_PER_DISPATCH);

			// For each virtual texture page being created this iteration,...
			for (const auto i : std::views::iota(0u, numPagesThisIteration))
			{
				// ...create a D3D12::Texture2D instance for it,...
				constexpr D3D12::Texture2DBuilder PAGE_BUILDER{ CreateVirtualTexturePageBuilder<TextureFormat, FilterMode>() };
				D3D12::Texture2D& pageTexture2D{ frameGraphBuilder.CreateTransientResource<D3D12::Texture2D>(PAGE_BUILDER) };

				// ...create the VirtualTexturePage instance which we will use to represent it later,...
				mPageArr.emplace_back(VirtualTexturePageInitializationInfo{
					.TextureSubResource{pageTexture2D.GetSubResource(0)},
					.PageCoordinates{currPageCoordinates},
					.StartingLogicalMipLevel{mipMapSubResource.GetSubResourceIndex()},
					.LogicalMipLevelCount = 1
				});

				// ...and cache the created D3D12::Texture2D instance for this iteration.
				outputPageTexturePtrArr.push_back(&pageTexture2D);

				// We also need to update currPageCoordinates to account for the page which we just created.
				++(currPageCoordinates.x);

				while (currPageCoordinates.x >= numPagesPerLogicalRow)
				{
					++(currPageCoordinates.y);
					currPageCoordinates.x -= numPagesPerLogicalRow;
				}
			}

			struct LargePageTilingInfo
			{
				std::unique_ptr<D3D12::DescriptorTableBuilder> OutputPageTableBuilderPtr;
				std::shared_ptr<D3D12::DescriptorTableBuilder> InputTextureTableBuilderPtr;
				DirectX::XMUINT2 StartingLogicalCoordinates;
				std::uint32_t OutputPageCount;
				D3D12::ConstantBufferSnapshot<MipLevelInfo> CurrMipLevelInfoSnapshot;
			};

			D3D12::RenderPass<D3D12::GPUCommandQueueType::DIRECT, LargePageTilingInfo> mipLevelPartitionPass{};
			mipLevelPartitionPass.SetRenderPassName("Virtual Texture Partitioning - Large Mip Level Partition Pass");

			LargePageTilingInfo tilingInfo{
				.OutputPageTableBuilderPtr{ std::make_unique<D3D12::DescriptorTableBuilder>(static_cast<std::uint32_t>(MAX_OUTPUT_PAGES_PER_DISPATCH)) },
				.InputTextureTableBuilderPtr{ mSrcTextureTableBuilderPtr },
				.StartingLogicalCoordinates{ currStartingCoordinates },
				.OutputPageCount{ static_cast<std::uint32_t>(numPagesThisIteration) },
				.CurrMipLevelInfoSnapshot{ currMipLevelInfoSubAllocation }
			};

			mipLevelPartitionPass.AddResourceDependency(mipMapSubResource, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			mipLevelPartitionPass.AddResourceDependency(currMipLevelInfoSubAllocation, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

			// Initialize the descriptor table entries for the output pages as appropriate.
			for (const auto i : std::views::iota(0u, MAX_OUTPUT_PAGES_PER_DISPATCH))
			{
				constexpr DXGI_FORMAT UAV_FORMAT = (IsSRGB<TextureFormat>() ? GetUAVFormatForSRGBFormat<TextureFormat>() : TextureFormat);
				
				if (i < outputPageTexturePtrArr.size()) [[likely]]
				{
					D3D12::Texture2DSubResource currPageSubResource{ outputPageTexturePtrArr[i]->GetSubResource(0) };

					mipLevelPartitionPass.AddResourceDependency(currPageSubResource, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
					tilingInfo.OutputPageTableBuilderPtr->CreateUnorderedAccessView(i, currPageSubResource.CreateUnorderedAccessView<UAV_FORMAT>());
				}
				else [[unlikely]]
					tilingInfo.OutputPageTableBuilderPtr->NullifyUnorderedAccessView<UAV_FORMAT, D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2D>(i);
			}

			mipLevelPartitionPass.SetInputData(std::move(tilingInfo));

			mipLevelPartitionPass.SetRenderPassCommands([] (D3D12::DirectContext& context, const LargePageTilingInfo& largeTilingInfo)
			{
				struct TilingInfo
				{
					DirectX::XMUINT2 StartingLogicalCoordinates;
					std::uint32_t OutputPageCount;
				};

				using RootParams = Brawler::RootParameters::VirtualTexturePageTiling;

				constexpr Brawler::PSOs::PSOID PSO_IDENTIFIER = GetLargeMipLevelPartitionPSOIdentifier<TextureFormat, FilterMode>();
				auto resourceBinder{ context.SetPipelineState<PSO_IDENTIFIER>() };

				{
					TilingInfo tilingInfoConstants{
						.StartingLogicalCoordinates{largeTilingInfo.StartingLogicalCoordinates},
						.OutputPageCount = largeTilingInfo.OutputPageCount
					};

					resourceBinder.BindRoot32BitConstants<RootParams::TILING_ROOT_CONSTANTS>(tilingInfoConstants);
				}

				resourceBinder.BindDescriptorTable<RootParams::OUTPUT_PAGES_TABLE>(largeTilingInfo.OutputPageTableBuilderPtr->GetDescriptorTable());
				resourceBinder.BindRootCBV<RootParams::MIP_LEVEL_INFO_CBV>(largeTilingInfo.CurrMipLevelInfoSnapshot.CreateRootConstantBufferView());

				// Even though std::shared_ptr does not provide thread-safe access, D3D12::DescriptorTableBuilder is, in fact, a
				// thread-safe class, so this works out fine.
				resourceBinder.BindDescriptorTable<RootParams::INPUT_TEXTURE_TABLE>(largeTilingInfo.InputTextureTableBuilderPtr->GetDescriptorTable());

				// As per the compute shader contract in VirtualTextureTiling.hlsl, we need to create one thread
				// for each texel in a page.
				static constexpr std::uint32_t THREAD_GROUP_SIZE_PER_XY_DIMENSION = 8;
				static constexpr DirectX::XMUINT2 TOTAL_PAGE_DIMENSIONS{ VirtualTextures::GetTotalPageDimensions<FilterMode>() };

				static constexpr std::uint32_t NUM_THREAD_GROUPS_X = (TOTAL_PAGE_DIMENSIONS.x / THREAD_GROUP_SIZE_PER_XY_DIMENSION) + std::min<std::uint32_t>(TOTAL_PAGE_DIMENSIONS.x % THREAD_GROUP_SIZE_PER_XY_DIMENSION, 1);
				static constexpr std::uint32_t NUM_THREAD_GROUPS_Y = (TOTAL_PAGE_DIMENSIONS.y / THREAD_GROUP_SIZE_PER_XY_DIMENSION) + std::min<std::uint32_t>(TOTAL_PAGE_DIMENSIONS.y % THREAD_GROUP_SIZE_PER_XY_DIMENSION, 1);

				context.Dispatch2D(NUM_THREAD_GROUPS_X, NUM_THREAD_GROUPS_Y);
			});

			mipLevelPartitionPassBundle.AddRenderPass(std::move(mipLevelPartitionPass));

			// Clear the cached array of D3D12::Texture2D* values for the next iteration.
			outputPageTexturePtrArr.clear();

			// Offset the starting logical coordinates to account for the pages which we created.
			{
				currStartingCoordinates.x += static_cast<std::uint32_t>(VirtualTextures::USEFUL_VIRTUAL_TEXTURE_PAGE_DIMENSIONS.x * numPagesThisIteration);

				const std::uint32_t pageColumnOffset = (currStartingCoordinates.x / static_cast<std::uint32_t>(currMipLevelDimensions));
				currStartingCoordinates.x = (currStartingCoordinates.x % static_cast<std::uint32_t>(currMipLevelDimensions));
				currStartingCoordinates.y += (pageColumnOffset * VirtualTextures::USEFUL_VIRTUAL_TEXTURE_PAGE_DIMENSIONS.y);
			}
			
			// Account for the pages which we just created.
			numPagesCreated += numPagesThisIteration;
		}

		frameGraphBuilder.AddRenderPassBundle(std::move(mipLevelPartitionPassBundle));
	}

	template <DXGI_FORMAT TextureFormat, VirtualTexturePageFilterMode FilterMode>
	void VirtualTexturePartitioner<TextureFormat, FilterMode>::MergeSmallMipMaps(D3D12::FrameGraphBuilder& frameGraphBuilder, const std::uint32_t firstMipLevelToMerge)
	{
		const std::uint32_t numMipLevelsToMerge = static_cast<std::uint32_t>(mSrcTexturePtr->GetResourceDescription().MipLevels - firstMipLevelToMerge);
		assert(numMipLevelsToMerge > 0);

		// Create the D3D12::Texture2D instance which will represent this virtual texture page.
		constexpr D3D12::Texture2DBuilder PAGE_BUILDER{ CreateVirtualTexturePageBuilder<TextureFormat, FilterMode>() };
		D3D12::Texture2D& outputPageTexture{ frameGraphBuilder.CreateTransientResource<D3D12::Texture2D>(PAGE_BUILDER) };

		// Create the VirtualTexturePage instance which will represent the final merged page.
		mPageArr.emplace_back(VirtualTexturePageInitializationInfo{
			.TextureSubResource{ outputPageTexture.GetSubResource(0) },
			.PageCoordinates{},
			.StartingLogicalMipLevel = firstMipLevelToMerge,
			.LogicalMipLevelCount = numMipLevelsToMerge
		});

		struct ConstantsInfo
		{
			std::uint32_t FirstMipLevelToBeMerged;
			std::uint32_t NumMipLevelsToMerge;
			std::uint32_t MaxLogicalSize;
		};

		struct MergeSmallMipMapsPassInfo
		{
			std::unique_ptr<D3D12::DescriptorTableBuilder> OutputTextureTableBuilderPtr;
			std::shared_ptr<D3D12::DescriptorTableBuilder> InputTextureTableBuilderPtr;
			ConstantsInfo RootConstantsInfo;
		};

		D3D12::RenderPass<D3D12::GPUCommandQueueType::DIRECT, MergeSmallMipMapsPassInfo> mipLevelMergePass{};
		mipLevelMergePass.SetRenderPassName("Virtual Texture Partitioning - Small Mip Level Merge Pass");

		D3D12::Texture2DSubResource outputPageSubResource{ outputPageTexture.GetSubResource(0) };
		mipLevelMergePass.AddResourceDependency(outputPageSubResource, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		for (const auto i : std::views::iota(firstMipLevelToMerge, numMipLevelsToMerge))
		{
			D3D12::Texture2DSubResource mergedMipLevelSubResource{ mSrcTexturePtr->GetSubResource(i) };
			mipLevelMergePass.AddResourceDependency(mergedMipLevelSubResource, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		}

		{
			MergeSmallMipMapsPassInfo passInfo{
				.OutputTextureTableBuilderPtr{ std::make_unique<D3D12::DescriptorTableBuilder>(1) },
				.InputTextureTableBuilderPtr{ mSrcTextureTableBuilderPtr },
				.RootConstantsInfo{
					.FirstMipLevelToBeMerged = firstMipLevelToMerge,
					.NumMipLevelsToMerge = numMipLevelsToMerge,
					.MaxLogicalSize = static_cast<std::uint32_t>(mSrcTexturePtr->GetResourceDescription().Width >> firstMipLevelToMerge)
				}
			};

			constexpr DXGI_FORMAT UAV_FORMAT = (IsSRGB<TextureFormat>() ? GetUAVFormatForSRGBFormat<TextureFormat>() : TextureFormat);
			passInfo.OutputTextureTableBuilderPtr->CreateUnorderedAccessView(0, outputPageTexture.CreateUnorderedAccessView<UAV_FORMAT>(0));

			mipLevelMergePass.SetInputData(std::move(passInfo));
		}
		
		mipLevelMergePass.SetRenderPassCommands([] (D3D12::DirectContext& context, const MergeSmallMipMapsPassInfo& passInfo)
		{
			constexpr Brawler::PSOs::PSOID PSO_IDENTIFIER = GetSmallMipLevelMergePSOIdentifier<TextureFormat, FilterMode>();
			auto resourceBinder{ context.SetPipelineState<PSO_IDENTIFIER>() };

			using RootParams = Brawler::RootParameters::VirtualTexturePageMerging;

			resourceBinder.BindRoot32BitConstants<RootParams::TILING_CONSTANTS>(passInfo.RootConstantsInfo);
			resourceBinder.BindDescriptorTable<RootParams::OUTPUT_TEXTURE_TABLE>(passInfo.OutputTextureTableBuilderPtr->GetDescriptorTable());
			resourceBinder.BindDescriptorTable<RootParams::INPUT_TEXTURE_TABLE>(passInfo.InputTextureTableBuilderPtr->GetDescriptorTable());

			// As per the compute shader contract in VirtualTexturePageMerging.hlsl, we need to create one thread
			// for each texel in a page.
			static constexpr std::uint32_t THREAD_GROUP_SIZE_PER_XY_DIMENSION = 8;
			static constexpr DirectX::XMUINT2 TOTAL_PAGE_DIMENSIONS{ VirtualTextures::GetTotalPageDimensions<FilterMode>() };

			static constexpr std::uint32_t NUM_THREAD_GROUPS_X = (TOTAL_PAGE_DIMENSIONS.x / THREAD_GROUP_SIZE_PER_XY_DIMENSION) + std::min<std::uint32_t>(TOTAL_PAGE_DIMENSIONS.x % THREAD_GROUP_SIZE_PER_XY_DIMENSION, 1);
			static constexpr std::uint32_t NUM_THREAD_GROUPS_Y = (TOTAL_PAGE_DIMENSIONS.y / THREAD_GROUP_SIZE_PER_XY_DIMENSION) + std::min<std::uint32_t>(TOTAL_PAGE_DIMENSIONS.y % THREAD_GROUP_SIZE_PER_XY_DIMENSION, 1);

			context.Dispatch2D(NUM_THREAD_GROUPS_X, NUM_THREAD_GROUPS_Y);
		});

		D3D12::RenderPassBundle mipLevelMergePassBundle{};
		mipLevelMergePassBundle.AddRenderPass(std::move(mipLevelMergePass));

		frameGraphBuilder.AddRenderPassBundle(std::move(mipLevelMergePassBundle));
	}
}

export namespace Brawler
{
	template <DXGI_FORMAT TextureFormat>
	using PointFilterVirtualTexturePartitioner = VirtualTexturePartitioner<TextureFormat, VirtualTexturePageFilterMode::POINT_FILTER>;

	template <DXGI_FORMAT TextureFormat>
	using BilinearFilterVirtualTexturePartitioner = VirtualTexturePartitioner<TextureFormat, VirtualTexturePageFilterMode::BILINEAR_FILTER>;

	template <DXGI_FORMAT TextureFormat>
	using TrilinearFilterVirtualTexturePartitioner = VirtualTexturePartitioner<TextureFormat, VirtualTexturePageFilterMode::TRILINEAR_FILTER>;

	template <DXGI_FORMAT TextureFormat>
	using AnisotropicFilterVirtualTexturePartitioner = VirtualTexturePartitioner<TextureFormat, VirtualTexturePageFilterMode::ANISOTROPIC_8X_FILTER>;
}