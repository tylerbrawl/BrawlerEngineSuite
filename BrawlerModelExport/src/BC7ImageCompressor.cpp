module;
#include <span>
#include <array>
#include <vector>
#include <cassert>
#include <ranges>
#include <optional>
#include <string>
#include <DxDef.h>
#include <DirectXTex.h>

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
import Brawler.D3D12.GPUResourceViews;

#pragma push_macro("max")
#undef max

#pragma push_macro("AddJob")
#undef AddJob

namespace
{
	static constexpr float BC7_COMPRESSION_ALPHA_WEIGHT = 1.0f;
	static constexpr std::uint64_t INVALID_COMPLETION_FRAME_NUMBER = std::numeric_limits<std::uint64_t>::max();
}

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
		mTableBuilderInfo()
	{}

	D3D12::BufferSubAllocationReservationHandle BC7ImageCompressor::AddCompressionRenderPasses(D3D12::FrameGraphBuilder& frameGraphBuilder)
	{
		CreateTransientResources(frameGraphBuilder);
		AddImageCompressionRenderPassBundles(frameGraphBuilder);

		// Revoke the BufferSubAllocationReservation from the OutputBufferSubAllocation. This
		// allows us to re-purpose the memory, such as for using its contents as the source
		// data for a TextureCopyBufferSubAllocation.
		return mResourceInfo.OutputBufferSubAllocation.RevokeReservation();
	}

	void BC7ImageCompressor::CreateTransientResources(D3D12::FrameGraphBuilder& frameGraphBuilder)
	{
		InitializeBufferResources(frameGraphBuilder);
	}

	void BC7ImageCompressor::AddImageCompressionRenderPassBundles(D3D12::FrameGraphBuilder& frameGraphBuilder)
	{
		assert(mResourceInfo.SourceTexturePtr != nullptr && "ERROR: BC7ImageCompressor::GetImageCompressionRenderPassBundles() was called before the compressor's transient resource could be created (i.e., before BC7ImageCompressor::CreateTransientResources())!");
		
		std::vector<D3D12::RenderPassBundle> createdBundleArr{};
		createdBundleArr.reserve(2);

		createdBundleArr.push_back(CreateResourceUploadRenderPassBundle());
		createdBundleArr.push_back(CreateCompressionRenderPassBundle());
		
		for (auto&& bundle : createdBundleArr)
			frameGraphBuilder.AddRenderPassBundle(std::move(bundle));
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

		const std::size_t numTotalBlocks = GetTotalBlockCount();

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

		{
			D3D12::BufferResource& uploadBufferResource{ frameGraphBuilder.CreateTransientResource<D3D12::BufferResource>(D3D12::BufferResourceInitializationInfo{
				.SizeInBytes = sizeof(ConstantsBC7),
				.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD
			}) };

			// Constant Buffer Copy Buffer
			std::optional<D3D12::StructuredBufferSubAllocation<ConstantsBC7>> optionalConstantBufferCopySubAllocation{ uploadBufferResource.CreateBufferSubAllocation<D3D12::StructuredBufferSubAllocation<ConstantsBC7>>() };
			assert(optionalConstantBufferCopySubAllocation.has_value());

			mResourceInfo.ConstantBufferCopySubAllocation = std::move(*optionalConstantBufferCopySubAllocation);
		}
	}

	void BC7ImageCompressor::InitializeDescriptorTableBuilders()
	{
		// The DirectXTex BC7 image compressor always creates an SRV of type DXGI_FORMAT_R8G8B8A8_UNORM during
		// BC7 image compression for the source texture.
		{
			assert(mInitInfo.SrcTextureSubResource.GetSubResourceIndex() == 0 && "ERROR: BC7 image compression should always be done on the highest mip level!");
			
			D3D12::Texture2DShaderResourceView<DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM> subResourceSRV{ mInitInfo.SrcTextureSubResource.GetGPUResource(), D3D12_TEX2D_SRV{
				.MostDetailedMip = mInitInfo.SrcTextureSubResource.GetSubResourceIndex(),
				.MipLevels = 1,
				.PlaneSlice = 0,
				.ResourceMinLODClamp = 0.0f
			} };
			mTableBuilderInfo.SourceTextureTableBuilder.CreateShaderResourceView(0, std::move(subResourceSRV));
		}

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
			struct ConstantBufferCopyInfo
			{
				D3D12::ConstantBufferSubAllocation<ConstantsBC7>& DestConstantBufferSubAllocation;
				D3D12::StructuredBufferSubAllocation<ConstantsBC7> ConstantBufferUploadSubAllocation;
				ConstantsBC7 ConstantsData;
			};

			D3D12::RenderPass<D3D12::GPUCommandQueueType::DIRECT, ConstantBufferCopyInfo> cbCopyRenderPass{};
			cbCopyRenderPass.SetRenderPassName("BC7 Image Compressor - Constant Buffer Copy");

			cbCopyRenderPass.AddResourceDependency(mResourceInfo.ConstantBufferSubAllocation, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
			cbCopyRenderPass.AddResourceDependency(mResourceInfo.ConstantBufferCopySubAllocation, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE);

			const Brawler::D3D12_RESOURCE_DESC& srcTextureDesc{ mInitInfo.SrcTextureSubResource.GetResourceDescription() };
			const std::uint32_t textureWidth = static_cast<std::uint32_t>(srcTextureDesc.Width);

			ConstantsBC7 cbData{
				.TextureWidth = textureWidth,
				.NumBlockX = static_cast<std::uint32_t>(std::max<std::size_t>(1, (textureWidth + 3) >> 2)),
				.Format = static_cast<std::uint32_t>(mInitInfo.DesiredFormat),
				.NumTotalBlocks = static_cast<std::uint32_t>(GetTotalBlockCount()),
				.AlphaWeight = BC7_COMPRESSION_ALPHA_WEIGHT
			};

			cbCopyRenderPass.SetInputData(ConstantBufferCopyInfo{
				.DestConstantBufferSubAllocation{mResourceInfo.ConstantBufferSubAllocation},
				.ConstantBufferUploadSubAllocation{std::move(mResourceInfo.ConstantBufferCopySubAllocation)},
				.ConstantsData{std::move(cbData)}
				});

			cbCopyRenderPass.SetRenderPassCommands([] (D3D12::DirectContext& context, const ConstantBufferCopyInfo& copyInfo)
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

		const std::size_t numTotalBlocks = GetTotalBlockCount();

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

				mode0Pass.AddResourceDependency(mInitInfo.SrcTextureSubResource, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				mode0Pass.AddResourceDependency(mResourceInfo.ConstantBufferSubAllocation, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				mode0Pass.AddResourceDependency(mResourceInfo.Error1BufferSubAllocation, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

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

					context.Dispatch1D(passInfo.ThreadGroupCount);
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

					compressionPass.AddResourceDependency(mInitInfo.SrcTextureSubResource, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
					compressionPass.AddResourceDependency(mResourceInfo.ConstantBufferSubAllocation, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

					if constexpr (CURRENT_ERROR_BINDING_MODE == ErrorBindingMode::ERROR1_SRV_ERROR2_UAV)
					{
						compressionPass.AddResourceDependency(mResourceInfo.Error1BufferSubAllocation, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
						compressionPass.AddResourceDependency(mResourceInfo.Error2BufferSubAllocation, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
					}
					else
					{
						compressionPass.AddResourceDependency(mResourceInfo.Error1BufferSubAllocation, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
						compressionPass.AddResourceDependency(mResourceInfo.Error2BufferSubAllocation, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
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

						context.Dispatch1D(passInfo.ThreadGroupCount);
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

				encodeBlockPass.AddResourceDependency(mInitInfo.SrcTextureSubResource, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				encodeBlockPass.AddResourceDependency(mResourceInfo.Error2BufferSubAllocation, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				encodeBlockPass.AddResourceDependency(mResourceInfo.ConstantBufferSubAllocation, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				encodeBlockPass.AddResourceDependency(mResourceInfo.OutputBufferSubAllocation, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

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

					context.Dispatch1D(passInfo.ThreadGroupCount);
				});

				compressionBundle.AddRenderPass(std::move(encodeBlockPass));
			}

			startBlockID += numBlocksInCurrBatch;
			numBlocksRemaining -= numBlocksInCurrBatch;
		}

		return compressionBundle;
	}

	std::size_t BC7ImageCompressor::GetTotalBlockCount() const
	{
		const Brawler::D3D12_RESOURCE_DESC& srcTextureDesc{ mInitInfo.SrcTextureSubResource.GetResourceDescription() };
		
		const std::size_t numXBlocks = std::max<std::size_t>(1, (srcTextureDesc.Width + 3) >> 2);
		const std::size_t numYBlocks = std::max<std::size_t>(1, (srcTextureDesc.Height + 3) >> 2);
		return (numXBlocks * numYBlocks);
	}
}

#pragma pop_macro("AddJob")
#pragma pop_macro("max")