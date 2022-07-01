module;
#include <vector>
#include <cassert>
#include <ranges>
#include <optional>
#include <bit>
#include <memory>
#include <DxDef.h>

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
import Brawler.D3D12.StructuredBufferElementRange;
import Brawler.D3D12.BufferCopyRegion;
import Brawler.D3D12.DescriptorTableBuilder;

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

	private:
		D3D12::Texture2D* mSrcTexturePtr;
		std::vector<VirtualTexturePage> mPageArr;
		std::vector<D3D12::ConstantBufferSubAllocation<MipLevelInfo>> mCBSubAllocationArr;
	};
}

// ---------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <DXGI_FORMAT TextureFormat, VirtualTexturePageFilterMode FilterMode>
	VirtualTexturePartitioner<TextureFormat, FilterMode>::VirtualTexturePartitioner(D3D12::Texture2D& srcTexture) :
		mSrcTexturePtr(&srcTexture),
		mPageArr(),
		mCBSubAllocationArr()
	{
		if constexpr (Util::General::IsDebugModeEnabled())
		{
			const Brawler::D3D12_RESOURCE_DESC& srcTextureDesc{ srcTexture.GetResourceDescription() };
			assert(Util::D3D12::IsSRVRTVDSVResourceCastLegal(srcTextureDesc.Format, TextureFormat) && "ERROR: An invalid DXGI_FORMAT value was provided when instantiating a VirtualTexturePartitioner instance!");
			assert(srcTextureDesc.Width == srcTextureDesc.Height && Util::Math::IsPowerOfTwo(srcTexture.Width) && "ERROR: Virtual textures must be square, and they must have dimensions which are powers of two!");
		}
	}

	template <DXGI_FORMAT TextureFormat, VirtualTexturePageFilterMode FilterMode>
	void VirtualTexturePartitioner<TextureFormat, FilterMode>::PartitionVirtualTexture(D3D12::FrameGraphBuilder& frameGraphBuilder)
	{

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

		dataCopyPass.SetInputData(std::move(dataCopyInfo));

		dataCopyPass.SetRenderPassCommands([] (D3D12::DirectContext& context, const ConstantBufferDataCopyInfo& copyInfo)
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
				mipLevelInfo.InputMipLevel = currMipLevel;
				mipLevelInfo.MipLevelLogicalSize = (copyInfo.MaxLogicalMipLevelDimensions >> currMipLevel);

				++currMipLevel;
			}

			{
				const std::span<const MipLevelInfo> uploadDataSpan{ mipLevelInfoArr };
				copyInfo.SrcDataSubAllocation.WriteStructuredBufferData(0, uploadDataSpan);
			}

			for (const auto i : std::views::iota(0u, numLargeMipLevels))
			{
				const D3D12::BufferCopyRegion destDataRegion{ copyInfo.DestBufferSnapshotArr[i].GetBufferCopyRegion() };
				const D3D12::BufferCopyRegion srcDataRegion{ copyInfo.SrcDataSubAllocation.GetBufferCopyRegion(StructuredBufferElementRange{
					.FirstElement = i,
					.NumElements = 1
				}) };

				context.CopyBufferToBuffer(destDataRegion, srcDataRegion);
			}
		});

		D3D12::RenderPassBundle dataCopyPassBundle{};
		dataCopyPassBundle.AddRenderPass(std::move(dataCopyPass));

		frameGraphBuilder.AddRenderPassBundle(std::move(dataCopyPassBundle));
	}

	template <DXGI_FORMAT TextureFormat, VirtualTexturePageFilterMode FilterMode>
	void VirtualTexturePartitioner::PartitionLargeMipMap(D3D12::FrameGraphBuilder& frameGraphBuilder, D3D12::Texture2DSubResource mipMapSubResource)
	{
		// The dimensions of the current mip level are (MipLevel0Dimensions) / 2^(CurrentMipLevel). However, since
		// we know that the source texture has power-of-two dimensions, we can optimize this calculation as follows:
		const std::size_t currMipLevelDimensions = (mSrcTexturePtr->GetResourceDescription().Width >> mipMapSubResource.GetSubResourceIndex());
		assert(static_cast<std::uint32_t>(currMipLevelDimensions) <= VirtualTextures::USEFUL_VIRTUAL_TEXTURE_PAGE_DIMENSIONS.x);

		// Since we only partition mip levels which have at least as many texels as the useful dimensions of a page,
		// and since both the useful page dimensions and highest mip level of the source texture are square powers
		// of two, we can easily calculate the number of pages needed as 
		// (CurrMipLevelWidth * CurrMipLevelHeight) / (UsefulPageWidth * UsefulPageHeight). The equations below
		// are optimized versions of this expression.
		static constexpr std::size_t NUM_USEFUL_PAGE_TEXELS = static_cast<std::size_t>(VirtualTextures::USEFUL_VIRTUAL_TEXTURE_PAGE_DIMENSIONS.x) * static_cast<std::size_t>(VirtualTextures::USEFUL_VIRTUAL_TEXTURE_PAGE_DIMENSIONS.y);
		static constexpr std::size_t SIZE_DIVISOR_EXPONENT = std::countr_zero(NUM_USEFUL_PAGE_TEXELS);
		const std::size_t numPagesToCreate = (currMipLevelDimensions << std::countr_zero(currMipLevelDimensions)) >> SIZE_DIVISOR_EXPONENT;

		const D3D12::ConstantBufferSubAllocation<MipLevelInfo>& currMipLevelInfoSubAllocation{ mCBSubAllocationArr[mipMapSubResource.GetSubResourceIndex()] };

		D3D12::RenderPassBundle mipLevelPartitionPassBundle{};
		std::size_t numPagesCreated = 0;

		static constexpr std::size_t MAX_OUTPUT_PAGES_PER_DISPATCH = 4;

		while (numPagesCreated < numPagesToCreate)
		{
			const std::size_t numPagesThisIteration = std::min<std::size_t>(numPagesToCreate - numPagesCreated, MAX_OUTPUT_PAGES_PER_DISPATCH);

		}
	}
}