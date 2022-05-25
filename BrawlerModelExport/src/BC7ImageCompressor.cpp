module;
#include <span>
#include <array>
#include <vector>
#include <cassert>
#include <ranges>
#include <optional>
#include <string>
#include <DirectXTex.h>
#include <DxDef.h>

module Brawler.BC7ImageCompressor;
import Brawler.D3D12.Texture2D;
import Brawler.JobSystem;
import Util.Coroutine;
import Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.FrameGraphBuilding;
import Brawler.D3D12.BufferResource;
import Brawler.D3D12.BufferResourceInitializationInfo;
import Brawler.D3D12.Texture2DBuilders;
import Brawler.D3D12.TextureSubResource;
import Brawler.D3D12.GPUResourceBinding;
import Brawler.D3D12.PipelineEnums;
import Util.Math;
import Util.Engine;
import Util.General;
import Brawler.RootSignatures.RootSignatureDefinition;

#pragma push_macro("max")
#undef max

#pragma push_macro("AddJob")
#undef AddJob

namespace
{
	static constexpr float BC7_COMPRESSION_ALPHA_WEIGHT = 1.0f;
	static constexpr std::uint64_t INVALID_COMPLETION_FRAME_NUMBER = std::numeric_limits<std::uint64_t>::max();
}

/*
namespace
{
	std::unique_ptr<Brawler::D3D12::ConstantBuffer<Brawler::ConstantsBC7>> CreateConstantBufferForCompression(const DirectX::Image& srcImage, const DXGI_FORMAT desiredFormat)
	{
		// Since the constant buffer will be read from multiple times but written to only once,
		// it is best to make it a default heap whose contents are initialized from a temporary
		// upload heap.

		std::unique_ptr<Brawler::D3D12::ConstantBuffer<Brawler::ConstantsBC7>> defaultCB{ std::make_unique<Brawler::D3D12::ConstantBuffer<Brawler::ConstantsBC7>>(Brawler::D3D12::ConstantBufferInitializationInfo{
			.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE,
			.AllocationDesc{
				.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT
			},

			// We need to set the resource state to D3D12_RESOURCE_STATE_COMMON to use a resource
			// on the copy queue.
			.InitialResourceState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON
		}) };

		std::unique_ptr<Brawler::D3D12::ConstantBuffer<Brawler::ConstantsBC7>> uploadCB{ std::make_unique<Brawler::D3D12::ConstantBuffer<Brawler::ConstantsBC7>>(Brawler::D3D12::ConstantBufferInitializationInfo{
			.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE,
			.AllocationDesc{
				.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD
			},

			// Here is where things get weird. The MSDN states that resources created in upload
			// heaps must start in the D3D12_RESOURCE_STATE_GENERIC_READ state. However, it also
			// states that all resources in a copy queue must start in the
			// D3D12_RESOURCE_STATE_COMMON state.
			//
			// Apparently, a small subset of resource transitions are valid on a copy queue. I am
			// going to do the following and hope that it works:
			//
			//   1. Create the resource in the D3D12_RESOURCE_STATE_GENERIC_READ state.
			//   2. Transition the resource to the D3D12_RESOURCE_STATE_COMMON state in the copy
			//      queue.
			//   3. Perform the copy operation.
			.InitialResourceState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ
		}) };

		{
			const std::uint32_t numXBlocks = std::max<std::uint32_t>(1, (static_cast<std::uint32_t>(srcImage.width) + 3) >> 2);
			const std::uint32_t numYBlocks = std::max<std::uint32_t>(1, (static_cast<std::uint32_t>(srcImage.height) + 3) >> 2);

			const Brawler::ConstantsBC7 constantsInfo{
				.TextureWidth = static_cast<std::uint32_t>(srcImage.width),
				.NumBlockX = numXBlocks,
				.Format = static_cast<std::uint32_t>(desiredFormat),
				.NumTotalBlocks = numXBlocks * numYBlocks,
				.AlphaWeight = 1.0f
			};
			
			Brawler::D3D12::ScopedGPUResourceMapping mappedUploadCBData{ *uploadCB, D3D12_RANGE{0, 0} };
			std::memcpy(mappedUploadCBData.Get(), &constantsInfo, sizeof(constantsInfo));
		}

		Brawler::D3D12::GPUJobGroup copyJobGroup{};
		const Brawler::D3D12::GPUResourceWriteHandle hDefaultCB{ copyJobGroup.CreateGPUResourceWriteHandle(*defaultCB) };
		const Brawler::D3D12::GPUResourceWriteHandle hUploadCB{ copyJobGroup.CreateGPUResourceWriteHandle(*uploadCB) };

		copyJobGroup.AddCopyJob([hDefaultCB, hUploadCB] (Brawler::D3D12::CopyContext& context)
		{
			// We need the resource present in the upload heap to be in the D3D12_RESOURCE_STATE_COMMON
			// resource before we can do any meaningful work with it in the copy queue.
			{
				Brawler::D3D12::GPUBarrierGroup barrierGroup{};
				barrierGroup.AddImmediateResourceTransition(hUploadCB, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON);

				context.ResourceBarrier(std::move(barrierGroup));
			}

			context.CopyResource(hDefaultCB, hUploadCB);
		});

		const Brawler::D3D12::GPUEventHandle hCopyCompletion{ copyJobGroup.ExecuteJobs() };
		hCopyCompletion.WaitForGPUExecution();

		return defaultCB;
	}

	Brawler::D3D12::Texture2D CreateTextureFromImage(const DirectX::Image& srcImage)
	{
		// The DirectXTex implementation intentionally avoids color-space conversions during compression.
		// I am not sure of why they do this, but we'll do the same. (We only need to check for this
		// format because this is the only valid sRGB input format.)
		const DXGI_FORMAT inputFormat = (srcImage.format == DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB ? DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM : srcImage.format);

		Brawler::D3D12::Texture2DInitializationInfo textureInitInfo{
			.Width = srcImage.width,
			.Height = static_cast<std::uint32_t>(srcImage.height),
			.MipLevels = 1,
			.Format = inputFormat,
			.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
			.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE,
			.AllocationDesc{
				.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD
			},
			.InitialResourceState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ
		};

		Brawler::D3D12::Texture2D uploadTexture{ textureInitInfo };

		{
			Brawler::D3D12::ScopedGPUResourceMapping uploadTextureMappedData{ uploadTexture, D3D12_RANGE{ 0, 0 } };
			std::memcpy(uploadTextureMappedData.Get(), srcImage.pixels, (srcImage.rowPitch * srcImage.height));
		}

		textureInitInfo.AllocationDesc.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
		textureInitInfo.InitialResourceState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;

		Brawler::D3D12::Texture2D defaultTexture{ textureInitInfo };

		Brawler::D3D12::GPUJobGroup textureCopyJobGroup{};
		const Brawler::D3D12::GPUResourceWriteHandle hDefaultTexture{ textureCopyJobGroup.CreateGPUResourceWriteHandle(defaultTexture) };
		const Brawler::D3D12::GPUResourceWriteHandle hUploadTexture{ textureCopyJobGroup.CreateGPUResourceWriteHandle(uploadTexture) };

		textureCopyJobGroup.AddCopyJob([hDefaultTexture, hUploadTexture] (Brawler::D3D12::CopyContext& context)
		{
			// We need the resource present in the upload heap to be in the D3D12_RESOURCE_STATE_COMMON
			// resource before we can do any meaningful work with it in the copy queue.
			{
				Brawler::D3D12::GPUBarrierGroup barrierGroup{};
				barrierGroup.AddImmediateResourceTransition(hUploadTexture, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON);

				context.ResourceBarrier(std::move(barrierGroup));
			}

			context.CopyResource(hDefaultTexture, hUploadTexture);
		});

		const Brawler::D3D12::GPUEventHandle hCopyCompletionEvent{ textureCopyJobGroup.ExecuteJobs() };
		hCopyCompletionEvent.WaitForGPUExecution();

		return defaultTexture;
	}
}
*/

namespace Brawler
{
	struct RootConstantsBC7
	{
		std::uint32_t ModeID;
		std::uint32_t StartBlockID;
	};
}

namespace Brawler
{
	BC7ImageCompressor::BC7ImageCompressor(InitInfo&& initInfo) :
		mInitInfo(std::move(initInfo)),
		mResourceInfo(),
		mTableBuilderInfo(),
		mReadbackBufferPtr(nullptr),
		mCompletionFrameNum(INVALID_COMPLETION_FRAME_NUMBER),
		mReadyForDeletion(false)
	{}

	/*
	bool BC7ImageCompressor::IsMipLevelSpanValid() const
	{
		if (mMipLevelImageSpan.empty())
			return false;

		struct ImageFormat
		{
			std::size_t Width;
			std::size_t Height;
			DXGI_FORMAT Format;
			std::size_t RowPitch;
			std::size_t SlicePitch;

			bool DoesImageMatchFormat(const DirectX::Image& image) const
			{
				return (image.width == Width && image.height == Height && image.format == Format &&
					image.rowPitch == RowPitch && image.slicePitch == SlicePitch);
			}
		};

		const ImageFormat requiredFormat{
			.Width = mMipLevelImageSpan[0].width,
			.Height = mMipLevelImageSpan[0].height,
			.Format = mMipLevelImageSpan[0].format,
			.RowPitch = mMipLevelImageSpan[0].rowPitch,
			.SlicePitch = mMipLevelImageSpan[0].slicePitch
		};

		for (const auto& image : mMipLevelImageSpan | std::views::drop(1))
		{
			if (!requiredFormat.DoesImageMatchFormat(image))
				return false;
		}

		return true;
	}
	*/

	void BC7ImageCompressor::CreateTransientResources(D3D12::FrameGraphBuilder& frameGraphBuilder)
	{
		InitializeSourceTextureResource(frameGraphBuilder);
		InitializeBufferResources(frameGraphBuilder);
	}

	std::vector<D3D12::RenderPassBundle> BC7ImageCompressor::GetImageCompressionRenderPassBundles()
	{
		assert(mResourceInfo.SourceTexturePtr != nullptr && "ERROR: BC7ImageCompressor::GetImageCompressionRenderPassBundles() was called before the compressor's transient resource could be created (i.e., before BC7ImageCompressor::CreateTransientResources())!");
		
		std::vector<D3D12::RenderPassBundle> createdBundleArr{};
		createdBundleArr.reserve(3);

		createdBundleArr.push_back(CreateResourceUploadRenderPassBundle());
		createdBundleArr.push_back(CreateCompressionRenderPassBundle());
		createdBundleArr.push_back(CreateResourceReadbackRenderPassBundle());

		// Although there are MAX_FRAMES_IN_FLIGHT frames in flight, we actually need to
		// wait until (MAX_FRAMES_IN_FLIGHT + 1) frames *after* these commands are recorded
		// in order to guarantee that data will be available in the readback heap. Here's why:
		//
		// Let frame N be the frame on which BC7ImageCompressor::GetImageCompressionRenderPassBundles()
		// is called, i.e., the frame on which the commands will be submitted to the GPU.
		//
		//   - [Frame N] BC7ImageCompressor Generates RenderPass Commands for GPU
		// 
		//   - [Frame N] Submit Commands to GPU -> [Frame N + 1]
		// 
		//   - [Frame N + 1] Before FrameGraph (N - 1) is Reset:
		//       - The BC7ImageCompressor is asked if the images are available. FrameGraph N has not been
		//         reset, so the answer is no, since we can't guarantee that the GPU has executed the commands.
		// 
		//   - [Frame N + 1] FrameGraph (N - 1) is Reset to Prepare for FrameGraph (N + 1)
		//
		//   - [Frame N + 1] BC7ImageCompressor has no RenderPass Commands for GPU - No Commands Submitted
		//
		//   - [Frame N + 1] Submit Commands to GPU -> [Frame N + 2]
		//
		//   - [Frame N + 2] Before FrameGraph N is Reset:
		//       - The BC7ImageCompressor is asked if the images are available. FrameGraph N has not been
		//         reset, so the answer is no, since we can't guarantee that the GPU has executed the commands.
		//
		//   - [Frame N + 2] FrameGraph N is Reset to Prepare for FrameGraph (N + 2)
		//
		//   - [Frame N + 2] BC7ImageCompressor has no RenderPass Commands for GPU - No Commands Submitted
		//
		//   - [Frame N + 2] Submit Commands to GPU -> [Frame N + 3]
		//
		//   - [Frame N + 3] Before FrameGraph (N + 1) is Reset:
		//       - The BC7ImageCompressor is asked if the images are available. Since FrameGraph N has been reset
		//         by this point, we know that the GPU has finished executing the commands, since a reset implies
		//         a GPU command flush for that FrameGraph. Thus, the answer is yes.
		//
		// This additional one frame delay is due, in part, to the lack of a way to determine exactly
		// when a GPU finishes executing a single command list; the Brawler Engine only tracks completion
		// on the GPU on a per-frame basis. In practice, however, I don't see this delay as being especially
		// harmful.

		mCompletionFrameNum = (Util::Engine::GetCurrentFrameNumber() + 3);

		return createdBundleArr;
	}

	bool BC7ImageCompressor::TryCopyCompressedImage(const DirectX::Image& destImage) const
	{
		assert(destImage.format == mInitInfo.DesiredFormat && destImage.width == mInitInfo.SrcImage.width && destImage.height == mInitInfo.SrcImage.height &&
			destImage.pixels != mInitInfo.SrcImage.pixels && "ERROR: An invalid DirectX::Image was provided to BC7ImageCompressor::TryCopyCompressedImage()!");

		// Make sure that the GPU has finished executing the commands.
		if (Util::Engine::GetCurrentFrameNumber() < mCompletionFrameNum)
			return false;

		std::size_t destRowPitch = 0;
		std::size_t destSlicePitch = 0;

		Util::General::CheckHRESULT(DirectX::ComputePitch(
			destImage.format,
			destImage.width,
			destImage.height,
			destRowPitch,
			destSlicePitch
		));

		const std::size_t numOutputBufferXBlocksPerRow = std::max<std::size_t>(1, (mInitInfo.SrcImage.width + 3) >> 2);
		const std::size_t numRows = std::max<std::size_t>(1, (mInitInfo.SrcImage.height + 3) >> 2);

		for (std::size_t i = 0; i < numRows; ++i)
		{
			const std::span<BufferBC7> destDataSpan{ reinterpret_cast<BufferBC7*>(destImage.pixels + (i * destRowPitch)), (destRowPitch / sizeof(BufferBC7)) };
			mResourceInfo.CPUOutputBufferSubAllocation.ReadStructuredBufferData(static_cast<std::uint32_t>(numOutputBufferXBlocksPerRow * i), destDataSpan);
		}

		return true;
	}

	void BC7ImageCompressor::MarkForDeletion()
	{
		mReadyForDeletion = true;
	}

	bool BC7ImageCompressor::ReadyForDeletion() const
	{
		return mReadyForDeletion;
	}

	void BC7ImageCompressor::InitializeBufferResources(D3D12::FrameGraphBuilder& frameGraphBuilder)
	{
		// It would be nice to put the output, error 1, and error 2 buffer sub-allocations all
		// inside one buffer. However, this is not actually possible, because it would require
		// the underlying buffer resource object to be placed into both the
		// D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE state and the
		// D3D12_RESOURCE_STATE_UNORDERED_ACCESS state. Thus, they must sadly be placed into
		// separate BufferResource instances.

		static constexpr auto INITIALIZE_SUB_ALLOCATION_LAMBDA = []<typename SubAllocationType, typename... Args>(D3D12::FrameGraphBuilder & builder, const D3D12::BufferResourceInitializationInfo & bufferInitInfo, Args&&... args) -> SubAllocationType
		{
			D3D12::BufferResource& bufferResource{ builder.CreateTransientResource<D3D12::BufferResource>(bufferInitInfo) };

			std::optional<SubAllocationType> optionalSubAllocation{ bufferResource.CreateBufferSubAllocation<SubAllocationType>(std::forward<Args>(args)...) };
			assert(optionalSubAllocation.has_value());

			return std::move(*optionalSubAllocation);
		};

		const std::size_t numXBlocks = std::max<std::size_t>(1, (mInitInfo.SrcImage.width + 3) >> 2);
		const std::size_t numYBlocks = std::max<std::size_t>(1, (mInitInfo.SrcImage.height + 3) >> 2);
		const std::size_t numTotalBlocks = (numXBlocks * numYBlocks);

		D3D12::BufferResourceInitializationInfo bufferInitInfo{
			.SizeInBytes = (numTotalBlocks * sizeof(BufferBC7)),
			.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT
		};

		// Output Buffer
		mResourceInfo.OutputBufferSubAllocation = INITIALIZE_SUB_ALLOCATION_LAMBDA.operator() < D3D12::StructuredBufferSubAllocation<BufferBC7> > (frameGraphBuilder, bufferInitInfo, numTotalBlocks);

		// Error 1 Buffer
		// The error 1 buffer has the exact same description as the output buffer.
		mResourceInfo.Error1BufferSubAllocation = INITIALIZE_SUB_ALLOCATION_LAMBDA.operator() < D3D12::StructuredBufferSubAllocation<BufferBC7> > (frameGraphBuilder, bufferInitInfo, numTotalBlocks);

		// Error 2 Buffer
		// The error 2 buffer has the exact same description as the output buffer.
		mResourceInfo.Error2BufferSubAllocation = INITIALIZE_SUB_ALLOCATION_LAMBDA.operator() < D3D12::StructuredBufferSubAllocation<BufferBC7> > (frameGraphBuilder, bufferInitInfo, numTotalBlocks);

		// Constants Buffer
		bufferInitInfo.SizeInBytes = sizeof(ConstantsBC7);

		mResourceInfo.ConstantBufferSubAllocation = INITIALIZE_SUB_ALLOCATION_LAMBDA.operator() < D3D12::ConstantBufferSubAllocation<ConstantsBC7> > (frameGraphBuilder, bufferInitInfo);

		// We can, however, create the two sub-allocations from an upload heap buffer from within the
		// same BufferResource.
		{
			const D3D12::TextureSubResource srcTextureSubResource{ mResourceInfo.SourceTexturePtr->GetSubResource() };
			std::uint64_t requiredSizeForTextureCopy = 0;

			Util::Engine::GetD3D12Device().GetCopyableFootprints1(
				&(srcTextureSubResource.GetResourceDescription()),
				srcTextureSubResource.GetSubResourceIndex(),
				1,
				0,
				nullptr,
				nullptr,
				nullptr,
				&requiredSizeForTextureCopy
			);

			D3D12::BufferResource& uploadBufferResource{ frameGraphBuilder.CreateTransientResource<D3D12::BufferResource>(D3D12::BufferResourceInitializationInfo{
				// The upload buffer must contain enough space for both the source texture and the constant buffer
				// upload. The latter has a smaller alignment, so we will add that one after the texture to reduce
				// the required space.
				.SizeInBytes = (Util::Math::AlignToPowerOfTwo(requiredSizeForTextureCopy, sizeof(ConstantsBC7)) + sizeof(ConstantsBC7)),

				.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD
			}) };

			// Source ModelTexture Copy Buffer
			std::optional<D3D12::TextureCopyBufferSubAllocation> optionalTextureCopySubAllocation{ uploadBufferResource.CreateBufferSubAllocation<D3D12::TextureCopyBufferSubAllocation>(srcTextureSubResource) };
			assert(optionalTextureCopySubAllocation.has_value());

			mResourceInfo.SourceTextureCopySubAllocation = std::move(*optionalTextureCopySubAllocation);

			// Constant Buffer Copy Buffer
			std::optional<D3D12::StructuredBufferSubAllocation<ConstantsBC7>> optionalConstantBufferCopySubAllocation{ uploadBufferResource.CreateBufferSubAllocation<D3D12::StructuredBufferSubAllocation<ConstantsBC7>>() };
			assert(optionalConstantBufferCopySubAllocation.has_value());

			mResourceInfo.ConstantBufferCopySubAllocation = std::move(*optionalConstantBufferCopySubAllocation);
		}

		// CPU Output Buffer
		//
		// This buffer needs to actually be a persistent resource, since it will need to outlive the
		// frame on which it is recorded into. That way, we can access its data on the CPU when the time
		// comes.
		bufferInitInfo.SizeInBytes = (numTotalBlocks * sizeof(BufferBC7));
		bufferInitInfo.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK;

		mReadbackBufferPtr = std::make_unique<D3D12::BufferResource>(bufferInitInfo);

		{
			std::optional<D3D12::StructuredBufferSubAllocation<BufferBC7>> optionalCPUOutputSubAllocation{ mReadbackBufferPtr->CreateBufferSubAllocation<D3D12::StructuredBufferSubAllocation<BufferBC7>>(numTotalBlocks) };
			assert(optionalCPUOutputSubAllocation.has_value());

			mResourceInfo.CPUOutputBufferSubAllocation = std::move(*optionalCPUOutputSubAllocation);
		}
	}

	void BC7ImageCompressor::InitializeSourceTextureResource(D3D12::FrameGraphBuilder& frameGraphBuilder)
	{
		D3D12::Texture2DBuilder sourceTextureBuilder{};
		sourceTextureBuilder.SetTextureDimensions(mInitInfo.SrcImage.width, mInitInfo.SrcImage.height);

		// The original DirectXTex implementation disables hardware colorspace conversion manually like this.
		// I am not sure why they do this.
		sourceTextureBuilder.SetTextureFormat(mInitInfo.SrcImage.format == DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB ? DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM : mInitInfo.SrcImage.format);

		sourceTextureBuilder.SetInitialResourceState(D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);

		mResourceInfo.SourceTexturePtr = &(frameGraphBuilder.CreateTransientResource<D3D12::Texture2D>(sourceTextureBuilder));
	}

	void BC7ImageCompressor::InitializeDescriptorTableBuilders()
	{
		mTableBuilderInfo.SourceTextureTableBuilder.CreateShaderResourceView(0, mResourceInfo.SourceTexturePtr->CreateShaderResourceView<DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM>());
		mTableBuilderInfo.Error1SRVTableBuilder.CreateShaderResourceView(0, mResourceInfo.Error1BufferSubAllocation.CreateShaderResourceViewForDescriptorTable());
		mTableBuilderInfo.Error1UAVTableBuilder.CreateUnorderedAccessView(0, mResourceInfo.Error1BufferSubAllocation.CreateUnorderedAccessViewForDescriptorTable());
		mTableBuilderInfo.Error2SRVTableBuilder.CreateShaderResourceView(0, mResourceInfo.Error2BufferSubAllocation.CreateShaderResourceViewForDescriptorTable());
		mTableBuilderInfo.Error2UAVTableBuilder.CreateUnorderedAccessView(0, mResourceInfo.Error2BufferSubAllocation.CreateUnorderedAccessViewForDescriptorTable());
		mTableBuilderInfo.OutputTableBuilder.CreateUnorderedAccessView(0, mResourceInfo.OutputBufferSubAllocation.CreateUnorderedAccessViewForDescriptorTable());
	}

	D3D12::RenderPassBundle BC7ImageCompressor::CreateResourceUploadRenderPassBundle()
	{
		// According to AMD, the COPY queue is optimized for transfers across the PCI-e bus. As usual, outside
		// articles provide a better insight to the D3D12 API than Microsoft's own documentation, with
		// Adam Sawicki writing a good article about how the D3D12_HEAP_TYPE enumeration abstracts the location
		// of GPU memory:
		//
		// https://asawicki.info/news_1755_untangling_direct3d_12_memory_heap_types_and_pools
		//
		// In one of the most bizarre naming choices I've seen in the D3D12 API, Microsoft decided to make L0
		// refer to system memory and L1 refer to GPU memory. Anyways, for discrete adapters, we find that
		// resources created in UPLOAD or READBACK heaps are created in L0 memory, meaning that transfers would
		// benefit from using the COPY queue.

		D3D12::RenderPassBundle uploadResourcesBundle{};

		{
			struct TextureCopyInfo
			{
				D3D12::TextureSubResource CopyDestTexture;
				D3D12::TextureCopyBufferSubAllocation& CopySrcSubAllocation;
				const DirectX::Image& SrcImage;
			};

			assert(mResourceInfo.SourceTexturePtr != nullptr);

			D3D12::RenderPass<D3D12::GPUCommandQueueType::COPY, TextureCopyInfo> textureCopyRenderPass{};
			textureCopyRenderPass.SetRenderPassName("BC7 Image Compressor - Source Texture Copy");

			textureCopyRenderPass.AddResourceDependency(*(mResourceInfo.SourceTexturePtr), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
			textureCopyRenderPass.AddResourceDependency(mResourceInfo.SourceTextureCopySubAllocation.GetBufferResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE);

			textureCopyRenderPass.SetInputData(TextureCopyInfo{
				.CopyDestTexture{mResourceInfo.SourceTexturePtr->GetSubResource()},
				.CopySrcSubAllocation{mResourceInfo.SourceTextureCopySubAllocation},
				.SrcImage{mInitInfo.SrcImage}
				});

			textureCopyRenderPass.SetRenderPassCommands([] (D3D12::CopyContext& context, const TextureCopyInfo& copyInfo)
			{
				// When we get to this point on the CPU, we know that the resources have been created on the GPU.
				// So, it is safe to both write to the buffer resource and perform the copy.

				std::size_t rowPitch = 0;
				std::size_t slicePitch = 0;

				Util::General::CheckHRESULT(DirectX::ComputePitch(
					copyInfo.SrcImage.format,
					copyInfo.SrcImage.width,
					copyInfo.SrcImage.height,
					rowPitch,
					slicePitch
				));

				for (std::size_t rowIndex = 0; rowIndex < copyInfo.SrcImage.height; ++rowIndex)
				{
					const std::span<const std::uint8_t> rowDataSpan{ (copyInfo.SrcImage.pixels + (rowPitch * rowIndex)), rowPitch };
					copyInfo.CopySrcSubAllocation.WriteTextureData(static_cast<std::uint32_t>(rowIndex), rowDataSpan);
				}

				context.CopyBufferToTexture(copyInfo.CopyDestTexture, copyInfo.CopySrcSubAllocation);
			});

			uploadResourcesBundle.AddRenderPass(std::move(textureCopyRenderPass));
		}

		{
			struct ConstantBufferCopyInfo
			{
				D3D12::ConstantBufferSubAllocation<ConstantsBC7>& DestConstantBufferSubAllocation;
				D3D12::StructuredBufferSubAllocation<ConstantsBC7> ConstantBufferUploadSubAllocation;
				ConstantsBC7 ConstantsData;
			};

			D3D12::RenderPass<D3D12::GPUCommandQueueType::COPY, ConstantBufferCopyInfo> cbCopyRenderPass{};
			cbCopyRenderPass.SetRenderPassName("BC7 Image Compressor - Constant Buffer Copy");

			cbCopyRenderPass.AddResourceDependency(mResourceInfo.ConstantBufferSubAllocation.GetBufferResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
			cbCopyRenderPass.AddResourceDependency(mResourceInfo.ConstantBufferCopySubAllocation.GetBufferResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE);

			ConstantsBC7 cbData{
				.TextureWidth = static_cast<std::uint32_t>(mInitInfo.SrcImage.width),
				.NumBlockX = static_cast<std::uint32_t>(std::max<std::size_t>(1, (mInitInfo.SrcImage.width + 3) >> 2)),
				.Format = static_cast<std::uint32_t>(mInitInfo.SrcImage.format),
				.NumTotalBlocks = static_cast<std::uint32_t>(GetTotalBlockCount()),
				.AlphaWeight = BC7_COMPRESSION_ALPHA_WEIGHT
			};

			cbCopyRenderPass.SetInputData(ConstantBufferCopyInfo{
				.DestConstantBufferSubAllocation{mResourceInfo.ConstantBufferSubAllocation},
				.ConstantBufferUploadSubAllocation{std::move(mResourceInfo.ConstantBufferCopySubAllocation)},
				.ConstantsData{std::move(cbData)}
				});

			cbCopyRenderPass.SetRenderPassCommands([] (D3D12::CopyContext& context, const ConstantBufferCopyInfo& copyInfo)
			{
				// Copy the data into the upload buffer.
				copyInfo.ConstantBufferUploadSubAllocation.WriteStructuredBufferData(0, std::span<const ConstantsBC7>{&(copyInfo.ConstantsData), 1});

				// Push the constants data to the default heap buffer.
				context.CopyBufferToBuffer(copyInfo.DestConstantBufferSubAllocation, copyInfo.ConstantBufferUploadSubAllocation);
			});

			uploadResourcesBundle.AddRenderPass(std::move(cbCopyRenderPass));
		}

		return uploadResourcesBundle;
	}

	D3D12::RenderPassBundle BC7ImageCompressor::CreateCompressionRenderPassBundle()
	{
		using RootParams = Brawler::RootParameters::BC6HBC7Compression;

		static constexpr std::uint32_t MAX_BLOCKS_IN_BATCH = 64;

		struct RootConstantsBC7
		{
			std::uint32_t ModeID;
			std::uint32_t StartBlockID;
		};

		struct CompressionPassInfo
		{
			RootConstantsBC7 RootConstants;
			ResourceInfo& Resources;
			DescriptorTableBuilderInfo& DescriptorTableBuilders;
			std::uint32_t ThreadGroupCount;
		};

		const std::size_t numXBlocks = std::max<std::size_t>(1, (mInitInfo.SrcImage.width + 3) >> 2);
		const std::size_t numYBlocks = std::max<std::size_t>(1, (mInitInfo.SrcImage.height + 3) >> 2);
		const std::size_t numTotalBlocks = (numXBlocks * numYBlocks);

		std::uint32_t numBlocksRemaining = static_cast<std::uint32_t>(numTotalBlocks);
		std::uint32_t startBlockID = 0;

		// Create the DescriptorTableBuilder instances here.
		InitializeDescriptorTableBuilders();

		D3D12::RenderPassBundle compressionBundle{};

		while (numBlocksRemaining > 0)
		{
			const std::uint32_t numBlocksInCurrBatch = std::min<std::uint32_t>(numBlocksRemaining, MAX_BLOCKS_IN_BATCH);

			// Mode 0?
			{
				RootConstantsBC7 mode0RootConstants{
					.ModeID = 0,
					.StartBlockID = startBlockID
				};

				D3D12::RenderPass<D3D12::GPUCommandQueueType::DIRECT, CompressionPassInfo> mode0Pass{};
				mode0Pass.SetRenderPassName("BC7 Image Compressor - Mode 0? Pass (BC7_TRY_MODE_456)");

				mode0Pass.AddResourceDependency(*(mResourceInfo.SourceTexturePtr), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				mode0Pass.AddResourceDependency(mResourceInfo.ConstantBufferSubAllocation.GetBufferResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				mode0Pass.AddResourceDependency(mResourceInfo.Error1BufferSubAllocation.GetBufferResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

				mode0Pass.SetInputData(CompressionPassInfo{
					.RootConstants{ std::move(mode0RootConstants) },
					.Resources{ mResourceInfo },
					.DescriptorTableBuilders{ mTableBuilderInfo },
					.ThreadGroupCount = std::max<std::uint32_t>(((numBlocksInCurrBatch + 3) / 4), 1)
					});

				mode0Pass.SetRenderPassCommands([] (D3D12::DirectContext& context, const CompressionPassInfo& passInfo)
				{
					auto resourceBinder{ context.SetPipelineState<Brawler::PSOs::PSOID::BC7_TRY_MODE_456>() };

					resourceBinder.BindDescriptorTable<RootParams::SOURCE_TEXTURE_SRV_TABLE>(passInfo.DescriptorTableBuilders.SourceTextureTableBuilder.GetDescriptorTable());

					// We require Resource Binding Tier 2 as a minimum, so we can leave INPUT_BUFFER_SRV_TABLE
					// unbound. (SRVs in descriptor tables do not need to be bound in this tier, and according
					// to NVIDIA, leaving descriptors unbound when it is possible improves performance.)

					resourceBinder.BindDescriptorTable<RootParams::OUTPUT_BUFFER_UAV_TABLE>(passInfo.DescriptorTableBuilders.Error1UAVTableBuilder.GetDescriptorTable());
					resourceBinder.BindRootCBV<RootParams::COMPRESSION_SETTINGS_CBV>(passInfo.Resources.ConstantBufferSubAllocation.CreateRootConstantBufferView());
					resourceBinder.BindRoot32BitConstants<RootParams::MODE_ID_AND_START_BLOCK_NUM_ROOT_CONSTANTS>(passInfo.RootConstants);

					context.Dispatch(
						passInfo.ThreadGroupCount,
						1,
						1
					);
				});

				compressionBundle.AddRenderPass(std::move(mode0Pass));
			}

			{
				// We can't make std::string_view a template parameter, so we'll just have to do this.
				constexpr std::array<std::string_view, 5> RENDER_PASS_NAME_ARRAY{
					"BC7 Image Compressor - Mode 1 Pass (BC7_TRY_MODE_137)",
					"BC7 Image Compressor - Mode 3 Pass (BC7_TRY_MODE_137)",
					"BC7 Image Compressor - Mode 7 Pass (BC7_TRY_MODE_137)",
					"BC7 Image Compressor - Mode 0 Pass (BC7_TRY_MODE_02)",
					"BC7 Image Compressor - Mode 2 Pass (BC7_TRY_MODE_02)"
				};

				const auto createRenderPassMode13702Lambda = [this, startBlockID, numBlocksInCurrBatch, &compressionBundle, &RENDER_PASS_NAME_ARRAY] <Brawler::PSOs::PSOID PSOIdentifier, std::uint32_t ModeID, std::size_t RenderPassNameIndex> ()
				{
					enum class ErrorBindingMode
					{
						ERROR1_SRV_ERROR2_UAV,
						ERROR1_UAV_ERROR2_SRV
					};

					static constexpr ErrorBindingMode CURRENT_ERROR_BINDING_MODE = ((ModeID == 1 || ModeID == 7 || ModeID == 2) ? ErrorBindingMode::ERROR1_SRV_ERROR2_UAV : ErrorBindingMode::ERROR1_UAV_ERROR2_SRV);

					D3D12::RenderPass<D3D12::GPUCommandQueueType::DIRECT, CompressionPassInfo> compressionPass{};

					constexpr std::string_view RENDER_PASS_NAME{ RENDER_PASS_NAME_ARRAY[RenderPassNameIndex] };
					compressionPass.SetRenderPassName(RENDER_PASS_NAME);

					compressionPass.AddResourceDependency(*(mResourceInfo.SourceTexturePtr), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
					compressionPass.AddResourceDependency(mResourceInfo.ConstantBufferSubAllocation.GetBufferResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

					if constexpr (CURRENT_ERROR_BINDING_MODE == ErrorBindingMode::ERROR1_SRV_ERROR2_UAV)
					{
						compressionPass.AddResourceDependency(mResourceInfo.Error1BufferSubAllocation.GetBufferResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
						compressionPass.AddResourceDependency(mResourceInfo.Error2BufferSubAllocation.GetBufferResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
					}
					else
					{
						compressionPass.AddResourceDependency(mResourceInfo.Error1BufferSubAllocation.GetBufferResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
						compressionPass.AddResourceDependency(mResourceInfo.Error2BufferSubAllocation.GetBufferResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
					}

					compressionPass.SetInputData(CompressionPassInfo{
						.RootConstants{
							.ModeID = ModeID,
							.StartBlockID = startBlockID
						},
						.Resources{ mResourceInfo },
						.DescriptorTableBuilders{ mTableBuilderInfo },
						.ThreadGroupCount = numBlocksInCurrBatch
						});

					compressionPass.SetRenderPassCommands([] (D3D12::DirectContext& context, const CompressionPassInfo& passInfo)
					{
						auto resourceBinder{ context.SetPipelineState<PSOIdentifier>() };

						resourceBinder.BindDescriptorTable<RootParams::SOURCE_TEXTURE_SRV_TABLE>(passInfo.DescriptorTableBuilders.SourceTextureTableBuilder.GetDescriptorTable());
						resourceBinder.BindRootCBV<RootParams::COMPRESSION_SETTINGS_CBV>(passInfo.Resources.ConstantBufferSubAllocation.CreateRootConstantBufferView());
						resourceBinder.BindRoot32BitConstants<RootParams::MODE_ID_AND_START_BLOCK_NUM_ROOT_CONSTANTS>(passInfo.RootConstants);

						if constexpr (CURRENT_ERROR_BINDING_MODE == ErrorBindingMode::ERROR1_SRV_ERROR2_UAV)
						{
							resourceBinder.BindDescriptorTable<RootParams::INPUT_BUFFER_SRV_TABLE>(passInfo.DescriptorTableBuilders.Error1SRVTableBuilder.GetDescriptorTable());
							resourceBinder.BindDescriptorTable<RootParams::OUTPUT_BUFFER_UAV_TABLE>(passInfo.DescriptorTableBuilders.Error2UAVTableBuilder.GetDescriptorTable());
						}
						else
						{
							resourceBinder.BindDescriptorTable<RootParams::INPUT_BUFFER_SRV_TABLE>(passInfo.DescriptorTableBuilders.Error2SRVTableBuilder.GetDescriptorTable());
							resourceBinder.BindDescriptorTable<RootParams::OUTPUT_BUFFER_UAV_TABLE>(passInfo.DescriptorTableBuilders.Error1UAVTableBuilder.GetDescriptorTable());
						}

						context.Dispatch(
							passInfo.ThreadGroupCount,
							1,
							1
						);
					});

					compressionBundle.AddRenderPass(std::move(compressionPass));
				};

				// Mode 1
				createRenderPassMode13702Lambda.operator()<Brawler::PSOs::PSOID::BC7_TRY_MODE_137, 1, 0>();

				// Mode 3
				createRenderPassMode13702Lambda.operator()<Brawler::PSOs::PSOID::BC7_TRY_MODE_137, 3, 1>();

				// Mode 7
				createRenderPassMode13702Lambda.operator()<Brawler::PSOs::PSOID::BC7_TRY_MODE_137, 7, 2>();

				// Mode 0
				createRenderPassMode13702Lambda.operator()<Brawler::PSOs::PSOID::BC7_TRY_MODE_02, 0, 3>();

				// Mode 2
				createRenderPassMode13702Lambda.operator()<Brawler::PSOs::PSOID::BC7_TRY_MODE_02, 2, 4>();
			}

			// Encode Block
			{
				D3D12::RenderPass<D3D12::GPUCommandQueueType::DIRECT, CompressionPassInfo> encodeBlockPass{};
				encodeBlockPass.SetRenderPassName("BC7 Image Compressor - Encode Block Pass (BC7_ENCODE_BLOCK)");

				encodeBlockPass.AddResourceDependency(*(mResourceInfo.SourceTexturePtr), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				encodeBlockPass.AddResourceDependency(mResourceInfo.Error2BufferSubAllocation.GetBufferResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				encodeBlockPass.AddResourceDependency(mResourceInfo.ConstantBufferSubAllocation.GetBufferResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				encodeBlockPass.AddResourceDependency(mResourceInfo.OutputBufferSubAllocation.GetBufferResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

				encodeBlockPass.SetInputData(CompressionPassInfo{
					.RootConstants{
						.ModeID = 2,  // Does this value really matter?
						.StartBlockID = startBlockID
					},
					.Resources{ mResourceInfo },
					.DescriptorTableBuilders{ mTableBuilderInfo },
					.ThreadGroupCount = std::max<std::uint32_t>(((numBlocksInCurrBatch + 3) / 4), 1)
				});

				encodeBlockPass.SetRenderPassCommands([] (D3D12::DirectContext& context, const CompressionPassInfo& passInfo)
				{
					auto resourceBinder{ context.SetPipelineState<Brawler::PSOs::PSOID::BC7_ENCODE_BLOCK>() };

					resourceBinder.BindDescriptorTable<RootParams::SOURCE_TEXTURE_SRV_TABLE>(passInfo.DescriptorTableBuilders.SourceTextureTableBuilder.GetDescriptorTable());
					resourceBinder.BindDescriptorTable<RootParams::INPUT_BUFFER_SRV_TABLE>(passInfo.DescriptorTableBuilders.Error2SRVTableBuilder.GetDescriptorTable());
					resourceBinder.BindDescriptorTable<RootParams::OUTPUT_BUFFER_UAV_TABLE>(passInfo.DescriptorTableBuilders.OutputTableBuilder.GetDescriptorTable());
					resourceBinder.BindRootCBV<RootParams::COMPRESSION_SETTINGS_CBV>(passInfo.Resources.ConstantBufferSubAllocation.CreateRootConstantBufferView());
					resourceBinder.BindRoot32BitConstants<RootParams::MODE_ID_AND_START_BLOCK_NUM_ROOT_CONSTANTS>(passInfo.RootConstants);

					context.Dispatch(
						passInfo.ThreadGroupCount,
						1,
						1
					);
				});

				compressionBundle.AddRenderPass(std::move(encodeBlockPass));
			}

			startBlockID += numBlocksInCurrBatch;
			numBlocksRemaining -= numBlocksInCurrBatch;
		}

		return compressionBundle;
	}

	D3D12::RenderPassBundle BC7ImageCompressor::CreateResourceReadbackRenderPassBundle()
	{
		struct ReadbackPassInfo
		{
			D3D12::StructuredBufferSubAllocation<BufferBC7>& BufferCopySrc;
			D3D12::StructuredBufferSubAllocation<BufferBC7>& BufferCopyDest;
		};

		D3D12::RenderPass<D3D12::GPUCommandQueueType::COPY, ReadbackPassInfo> readbackPass{};
		readbackPass.SetRenderPassName("BC7 Image Compressor - Read-Back Buffer Copy");

		readbackPass.AddResourceDependency(mResourceInfo.CPUOutputBufferSubAllocation.GetBufferResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
		readbackPass.AddResourceDependency(mResourceInfo.OutputBufferSubAllocation.GetBufferResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE);

		readbackPass.SetInputData(ReadbackPassInfo{
			.BufferCopySrc{mResourceInfo.OutputBufferSubAllocation},
			.BufferCopyDest{mResourceInfo.CPUOutputBufferSubAllocation}
		});

		readbackPass.SetRenderPassCommands([] (D3D12::CopyContext& context, const ReadbackPassInfo& passInfo)
		{
			// The NVIDIA Debug Layer is shooting out a false positive error for the underlying CopyBufferRegion()
			// call. I know that it is a false positive because it complains about passInfo.BufferCopySrc.GetBufferResource()
			// still being in the UNORDERED_ACCESS state, but a call to GPUCommandContext::DebugResourceBarrier()
			// transitioning the resource from UNORDERED_ACCESS to COPY_SOURCE reveals that it was actually already in the
			// COPY_SOURCE state, as we expect. (NOTE: If you want to test the resource barrier yourself, you will
			// need to change this to a DIRECT RenderPass, because the COPY queue does not normally support this
			// transition.)
			//
			// My disappointment is immeasurable, and my day is ruined.
			
			context.CopyBufferToBuffer(passInfo.BufferCopyDest, passInfo.BufferCopySrc);
		});

		D3D12::RenderPassBundle readbackPassBundle{};
		readbackPassBundle.AddRenderPass(std::move(readbackPass));

		return readbackPassBundle;
	}

	std::size_t BC7ImageCompressor::GetTotalBlockCount() const
	{
		const std::size_t numXBlocks = std::max<std::size_t>(1, (mInitInfo.SrcImage.width + 3) >> 2);
		const std::size_t numYBlocks = std::max<std::size_t>(1, (mInitInfo.SrcImage.height + 3) >> 2);
		return (numXBlocks * numYBlocks);
	}

	/*
	void BC7ImageCompressor::CompressImage(DirectX::Image& destImage)
	{
		assert(destImage.format == mDesiredFormat && "ERROR: A DirectX::Image with an unexpected format was provided for BC7ImageCompressor::CompressImage()!");
		
		D3D12::Texture2D srcTexture{ CreateTextureFromImage(*mSrcImagePtr) };

		D3D12::PerFrameDescriptorTable srcTextureSRV{};
		{
			D3D12::DescriptorTableBuilder srcTextureSRVBuilder{ 1 };
			srcTextureSRVBuilder.CreateShaderResourceView(0, srcTexture);

			srcTextureSRV = srcTextureSRVBuilder.FinalizeDescriptorTable();
		}

		// To avoid creating a lambda function which captures a lot of variables (thus reducing
		// the likelihood that the internal std::function is allocated on the heap), we create
		// a struct containing our relevant information and capture that by reference, instead.
		struct CompressionContext
		{
			std::uint32_t NumTotalBlocks;

			D3D12::GPUResourceWriteHandle HOutputBuffer;
			const D3D12::PerFrameDescriptorTable& OutputUAV;

			D3D12::GPUResourceWriteHandle HError1Buffer;
			const D3D12::PerFrameDescriptorTable& Error1SRV;
			const D3D12::PerFrameDescriptorTable& Error1UAV;

			D3D12::GPUResourceWriteHandle HError2Buffer;
			const D3D12::PerFrameDescriptorTable& Error2SRV;
			const D3D12::PerFrameDescriptorTable& Error2UAV;

			D3D12::GPUResourceReadHandle HConstantBuffer;

			D3D12::GPUResourceWriteHandle HOutputCPUBuffer;

			D3D12::GPUResourceReadHandle HSrcTexture;
			const D3D12::PerFrameDescriptorTable& SrcTextureSRV;
		};

		const std::uint32_t numXBlocks = std::max<std::uint32_t>(1, (static_cast<std::uint32_t>(mSrcImagePtr->width) + 3) >> 2);
		const std::uint32_t numYBlocks = std::max<std::uint32_t>(1, (static_cast<std::uint32_t>(mSrcImagePtr->height) + 3) >> 2);

		Brawler::D3D12::GPUJobGroup compressionJobGroup{};
		const CompressionContext compressionContext{
			.NumTotalBlocks{numXBlocks * numYBlocks},

			.HOutputBuffer{compressionJobGroup.CreateGPUResourceWriteHandle(*mOutputBuffer)},
			.OutputUAV{mOutputUAV},

			.HError1Buffer{compressionJobGroup.CreateGPUResourceWriteHandle(*mError1Buffer)},
			.Error1SRV{mError1SRV},
			.Error1UAV{mError1UAV},

			.HError2Buffer{compressionJobGroup.CreateGPUResourceWriteHandle(*mError2Buffer)},
			.Error2SRV{mError2SRV},
			.Error2UAV{mError2UAV},

			.HConstantBuffer{compressionJobGroup.CreateGPUResourceReadHandle(*mConstantBuffer)},

			.HOutputCPUBuffer{compressionJobGroup.CreateGPUResourceWriteHandle(*mOutputCPUBuffer)},

			.HSrcTexture{compressionJobGroup.CreateGPUResourceReadHandle(srcTexture)},
			.SrcTextureSRV{srcTextureSRV}
		};

		compressionJobGroup.AddComputeJob([&compressionContext] (Brawler::D3D12::ComputeContext& computeContext)
		{
			using RootParams = Brawler::RootParameters::BC6HBC7Compression;

			static constexpr std::uint32_t MAX_BLOCKS_IN_BATCH = 64;

			std::uint32_t blocksRemaining = compressionContext.NumTotalBlocks;
			std::uint32_t startBlockID = 0;

			bool initialResourcesBound = false;

			std::optional<D3D12::GPUSplitBarrierToken> error1SplitBarrierToken{};
			std::optional<D3D12::GPUSplitBarrierToken> error2SplitBarrierToken{};

			while (blocksRemaining > 0)
			{
				const std::uint32_t numBlocksInCurrentBatch = std::min<std::uint32_t>(blocksRemaining, MAX_BLOCKS_IN_BATCH);

				// PSOID::BC7_TRY_MODE_456
				{
					auto resourceBinder{ computeContext.SetPipelineState<Brawler::PSOs::PSOID::BC7_TRY_MODE_456>() };

					if (!initialResourcesBound)
					{
						resourceBinder.BindDescriptorTable<RootParams::SOURCE_TEXTURE_SRV_TABLE>(compressionContext.SrcTextureSRV);
						resourceBinder.BindRootCBV<RootParams::COMPRESSION_SETTINGS_CBV>(compressionContext.HConstantBuffer);

						initialResourcesBound = true;
					}

					if (error1SplitBarrierToken.has_value())
					{
						D3D12::GPUBarrierGroup barrierGroup{};
						barrierGroup.EndSplitResourceTransition(std::move(*error1SplitBarrierToken));

						computeContext.ResourceBarrier(std::move(barrierGroup));

						error1SplitBarrierToken.reset();
					}

					resourceBinder.BindDescriptorTable<RootParams::OUTPUT_BUFFER_UAV_TABLE>(compressionContext.Error1UAV);
					resourceBinder.BindRoot32BitConstants<RootParams::MODE_ID_AND_START_BLOCK_NUM_ROOT_CONSTANTS>(RootConstantsBC7{
						.ModeID = 0,
						.StartBlockID = startBlockID
						});

					computeContext.Dispatch(
						std::max<std::uint32_t>((numBlocksInCurrentBatch + 3) / 4, 1),
						1,
						1
					);

					D3D12::GPUBarrierGroup barrierGroup{};
					barrierGroup.AddImmediateResourceTransition(compressionContext.HError1Buffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

					if (error2SplitBarrierToken.has_value())
					{
						barrierGroup.EndSplitResourceTransition(std::move(*error2SplitBarrierToken));
						error2SplitBarrierToken.reset();
					}

					computeContext.ResourceBarrier(std::move(barrierGroup));
				}

				// PSOID::BC7_TRY_MODE_137
				{
					auto resourceBinder{ computeContext.SetPipelineState<Brawler::PSOs::PSOID::BC7_TRY_MODE_137>() };

					resourceBinder.BindDescriptorTable<RootParams::INPUT_BUFFER_SRV_TABLE>(compressionContext.Error1SRV);
					resourceBinder.BindDescriptorTable<RootParams::OUTPUT_BUFFER_UAV_TABLE>(compressionContext.Error2UAV);
					resourceBinder.BindRoot32BitConstants<RootParams::MODE_ID_AND_START_BLOCK_NUM_ROOT_CONSTANTS>(RootConstantsBC7{
						.ModeID = 1,
						.StartBlockID = startBlockID
						});

					computeContext.Dispatch(numBlocksInCurrentBatch, 1, 1);

					{
						D3D12::GPUBarrierGroup barrierGroup{};
						barrierGroup.AddImmediateResourceTransition(compressionContext.HError1Buffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
						barrierGroup.AddImmediateResourceTransition(compressionContext.HError2Buffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

						computeContext.ResourceBarrier(std::move(barrierGroup));
					}

					resourceBinder.BindDescriptorTable<RootParams::INPUT_BUFFER_SRV_TABLE>(compressionContext.Error2SRV);
					resourceBinder.BindDescriptorTable<RootParams::OUTPUT_BUFFER_UAV_TABLE>(compressionContext.Error1UAV);
					resourceBinder.BindRoot32BitConstants<RootParams::MODE_ID_AND_START_BLOCK_NUM_ROOT_CONSTANTS>(RootConstantsBC7{
						.ModeID = 3,
						.StartBlockID = startBlockID
						});

					computeContext.Dispatch(numBlocksInCurrentBatch, 1, 1);

					{
						D3D12::GPUBarrierGroup barrierGroup{};
						barrierGroup.AddImmediateResourceTransition(compressionContext.HError1Buffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
						barrierGroup.AddImmediateResourceTransition(compressionContext.HError2Buffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

						computeContext.ResourceBarrier(std::move(barrierGroup));
					}

					resourceBinder.BindDescriptorTable<RootParams::INPUT_BUFFER_SRV_TABLE>(compressionContext.Error1SRV);
					resourceBinder.BindDescriptorTable<RootParams::OUTPUT_BUFFER_UAV_TABLE>(compressionContext.Error2UAV);
					resourceBinder.BindRoot32BitConstants<RootParams::MODE_ID_AND_START_BLOCK_NUM_ROOT_CONSTANTS>(RootConstantsBC7{
						.ModeID = 7,
						.StartBlockID = startBlockID
						});

					computeContext.Dispatch(numBlocksInCurrentBatch, 1, 1);

					{
						D3D12::GPUBarrierGroup barrierGroup{};
						barrierGroup.AddImmediateResourceTransition(compressionContext.HError1Buffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
						barrierGroup.AddImmediateResourceTransition(compressionContext.HError2Buffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

						computeContext.ResourceBarrier(std::move(barrierGroup));
					}
				}

				// PSOID::BC7_TRY_MODE_02
				{
					auto resourceBinder{ computeContext.SetPipelineState<Brawler::PSOs::PSOID::BC7_TRY_MODE_02>() };

					resourceBinder.BindDescriptorTable<RootParams::INPUT_BUFFER_SRV_TABLE>(compressionContext.Error2SRV);
					resourceBinder.BindDescriptorTable<RootParams::OUTPUT_BUFFER_UAV_TABLE>(compressionContext.Error1UAV);
					resourceBinder.BindRoot32BitConstants<RootParams::MODE_ID_AND_START_BLOCK_NUM_ROOT_CONSTANTS>(RootConstantsBC7{
						.ModeID = 0,
						.StartBlockID = startBlockID
						});

					computeContext.Dispatch(numBlocksInCurrentBatch, 1, 1);

					{
						D3D12::GPUBarrierGroup barrierGroup{};
						barrierGroup.AddImmediateResourceTransition(compressionContext.HError1Buffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
						barrierGroup.AddImmediateResourceTransition(compressionContext.HError2Buffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

						computeContext.ResourceBarrier(std::move(barrierGroup));
					}

					resourceBinder.BindDescriptorTable<RootParams::INPUT_BUFFER_SRV_TABLE>(compressionContext.Error1SRV);
					resourceBinder.BindDescriptorTable<RootParams::OUTPUT_BUFFER_UAV_TABLE>(compressionContext.Error2UAV);
					resourceBinder.BindRoot32BitConstants<RootParams::MODE_ID_AND_START_BLOCK_NUM_ROOT_CONSTANTS>(RootConstantsBC7{
						.ModeID = 2,
						.StartBlockID = startBlockID
						});

					computeContext.Dispatch(numBlocksInCurrentBatch, 1, 1);

					{
						D3D12::GPUBarrierGroup barrierGroup{};
						barrierGroup.AddImmediateResourceTransition(compressionContext.HError2Buffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

						// We can split the transition of the error 1 buffer to UAV.
						if ((blocksRemaining - numBlocksInCurrentBatch) > 0)
							error1SplitBarrierToken = barrierGroup.BeginSplitResourceTransition(compressionContext.HError1Buffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

						// I'm pretty sure that we don't need a UAV barrier for the output here, since we are writing into
						// separate blocks... well, I'm assuming that we are, anyways.

						computeContext.ResourceBarrier(std::move(barrierGroup));
					}
				}

				// PSOID::BC7_ENCODE_BLOCK
				{
					auto resourceBinder{ computeContext.SetPipelineState<Brawler::PSOs::PSOID::BC7_ENCODE_BLOCK>() };

					resourceBinder.BindDescriptorTable<RootParams::INPUT_BUFFER_SRV_TABLE>(compressionContext.Error2SRV);
					resourceBinder.BindDescriptorTable<RootParams::OUTPUT_BUFFER_UAV_TABLE>(compressionContext.OutputUAV);

					computeContext.Dispatch(
						std::max<std::uint32_t>((numBlocksInCurrentBatch + 3) / 4, 1),
						1,
						1
					);

					if ((blocksRemaining - numBlocksInCurrentBatch) > 0)
					{
						D3D12::GPUBarrierGroup barrierGroup{};
						error2SplitBarrierToken = barrierGroup.BeginSplitResourceTransition(compressionContext.HError2Buffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

						computeContext.ResourceBarrier(std::move(barrierGroup));
					}
				}

				startBlockID += numBlocksInCurrentBatch;
				blocksRemaining -= numBlocksInCurrentBatch;
			}

			{
				D3D12::GPUBarrierGroup barrierGroup{};
				barrierGroup.AddImmediateResourceTransition(compressionContext.HOutputBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE);

				computeContext.ResourceBarrier(std::move(barrierGroup));
			}

			// Copy the buffer into the CPU-readable buffer resource.
			computeContext.CopyResource(compressionContext.HOutputCPUBuffer, compressionContext.HOutputBuffer);
		});

		D3D12::GPUEventHandle hCompletionEvent{ compressionJobGroup.ExecuteJobs() };
		hCompletionEvent.WaitForGPUExecution();

		// Now, we can finally copy the compressed texture from the read-back buffer into a DirectX::Image.

		{
			D3D12::ScopedGPUResourceMapping<std::uint8_t*> outputCPUMappedData{*mOutputCPUBuffer, D3D12::ScopedGPUResourceMapping<std::uint8_t*>::GenericReadRange::READ_ENTIRE_SUBRESOURCE};

			// Copy the buffer into the DirectX::Image. The original DirectXTex implementation did this row by row,
			// but I'm not even sure that's necessary.

			const std::size_t rowSizeInBytes = numXBlocks * sizeof(BufferBC7);
			assert(rowSizeInBytes == destImage.rowPitch);
			assert(numYBlocks == destImage.height);

			// If those two assertions pass and destImage is in the expected format (which we checked earlier), then
			// theoretically, it should be legal to just copy everything over.
			std::memcpy(destImage.pixels, outputCPUMappedData.Get(), (rowSizeInBytes * numYBlocks));
		}

		// The original DirectXTex implementation would work in the following manner:
		//
		// Update Constant Buffer -> Bind Resources -> Execute Shader -> Update Constant Buffer -> ...
		//
		// If we were to use the same process (i.e., updating the constant buffer after each shader execution),
		// we would need to submit a command list and wait for it to finish prior to each constant
		// buffer update after the first. This goes against the best practices of the DirectX 12 API,
		// which (according to https://gpuopen.com/performance/) include making command lists large
		// enough to make the kernel call and implicit GPU synchronization required for command list
		// submission worth the cost.
		//
		// Instead, we will create all of the constant buffers up front and use these during command
		// list execution. That way, not only can we do everything we need to in one command list, but
		// we can also reduce the cost of resource bindings by only binding what we need to for the next
		// shader execution.
		//
		// Of course, at the time of writing this, I am still very much a beginner with the DirectX 12
		// API. The best thing to do would be to use tools like PIX for benchmarking and recommendations.
	}

	void BC7ImageCompressor::InitializeBuffers()
	{
		const std::size_t width = mSrcImagePtr->width;
		const std::size_t height = mSrcImagePtr->height;
		
		const std::size_t numXBlocks = std::max<std::size_t>(1, (width + 3) >> 2);
		const std::size_t numYBlocks = std::max<std::size_t>(1, (height + 3) >> 2);

		const std::size_t numTotalBlocks = numXBlocks * numYBlocks;

		{
			const D3D12::StructuredBufferInitializationInfo bufferInitInfo{
				.NumElements = numTotalBlocks,
				.ResourceFlags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
				.AllocationDesc{
					.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT
				},
				.InitialResourceState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS
			};

			mOutputBuffer = std::make_unique<D3D12::StructuredBuffer<BufferBC7>>(bufferInitInfo);
			mError1Buffer = std::make_unique<D3D12::StructuredBuffer<BufferBC7>>(bufferInitInfo);
			mError2Buffer = std::make_unique<D3D12::StructuredBuffer<BufferBC7>>(bufferInitInfo);
		}

		// Construct the constant buffer asynchronously.
		std::atomic<bool> cbInitialized{ false };

		Brawler::JobGroup cbInitializationJobGroup{};
		cbInitializationJobGroup.Reserve(1);

		cbInitializationJobGroup.AddJob([this, &cbInitialized] ()
		{
			// We only need to pass the first DirectX::Image in the span, since they will all have the same
			// format.

			mConstantBuffer = CreateConstantBufferForCompression(*mSrcImagePtr, mDesiredFormat);
			cbInitialized.store(true);
		});

		cbInitializationJobGroup.ExecuteJobsAsync();

		// While we wait for the constant buffer to be initialized, we can take care of the rest of the
		// buffers here.

		{
			const D3D12::StructuredBufferInitializationInfo bufferInitInfo{
				.NumElements = numTotalBlocks,
				.ResourceFlags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE,
				.AllocationDesc{
					.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK
				},
				.InitialResourceState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST
			};

			mOutputCPUBuffer = std::make_unique<D3D12::StructuredBuffer<BufferBC7>>(bufferInitInfo);
		}

		{
			D3D12::DescriptorTableBuilder error1SRVBuilder{ 1 };
			error1SRVBuilder.CreateShaderResourceView(0, *mError1Buffer);

			mError1SRV = error1SRVBuilder.FinalizeDescriptorTable();
		}

		{
			D3D12::DescriptorTableBuilder error2SRVBuilder{ 1 };
			error2SRVBuilder.CreateShaderResourceView(0, *mError2Buffer);

			mError2SRV = error2SRVBuilder.FinalizeDescriptorTable();
		}

		{
			D3D12::DescriptorTableBuilder outputUAVBuilder{ 1 };
			outputUAVBuilder.CreateUnorderedAccessView(0, *mOutputBuffer);

			mOutputUAV = outputUAVBuilder.FinalizeDescriptorTable();
		}

		{
			D3D12::DescriptorTableBuilder error1UAVBuilder{ 1 };
			error1UAVBuilder.CreateUnorderedAccessView(0, *mError1Buffer);

			mError1UAV = error1UAVBuilder.FinalizeDescriptorTable();
		}

		{
			D3D12::DescriptorTableBuilder error2UAVBuilder{ 1 };
			error2UAVBuilder.CreateUnorderedAccessView(0, *mError2Buffer);

			mError2UAV = error2UAVBuilder.FinalizeDescriptorTable();
		}

		// Before we can continue, we need to wait for the constant buffer to be created.
		while (!cbInitialized.load())
			Util::Coroutine::TryExecuteJob();
	}
	*/
}

#pragma pop_macro("AddJob")
#pragma pop_macro("max")