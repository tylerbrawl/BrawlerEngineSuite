module;
#include <cassert>
#include <memory>
#include <vector>
#include <optional>
#include <ranges>
#include <DxDef.h>

module Brawler.OpaqueDiffuseModelTextureResolver;
import Util.Engine;
import Util.Math;
import Brawler.D3D12.BufferResourceInitializationInfo;
import Brawler.D3D12.FrameGraphBuilding;
import Brawler.D3D12.BufferSubAllocationReservation;

namespace Brawler
{
	void OpaqueDiffuseModelTextureResolver::AddReadBackBufferCopyRenderPasses(TextureResolutionContext& context)
	{
		// STEP 4
		//
		// Copy the generated textures into a persistent BufferResource in a READBACK heap.
		assert(!context.HBC7TextureDataReservationArr.empty());

		mBC7BufferSubAllocationArr.reserve(context.HBC7TextureDataReservationArr.size());

		std::size_t requiredReadBackBufferSize = 0;

		// Add the sizes of each BufferSubAllocationReservation returned as a result of BC7 image
		// compression to the size of the read-back buffer. We also need to account for the alignment
		// of StructuredBufferSubAllocation<BufferBC7> instances, since these are what will be
		// written into.

		constexpr std::size_t REQUIRED_BUFFER_BC7_SUB_ALLOCATION_ALIGNMENT = sizeof(BufferBC7);

		for (const auto& hBC7SubResourceReservation : context.HBC7TextureDataReservationArr)
		{
			// We want to ensure that we can create another StructuredBufferSubAllocation<BufferBC7> for
			// hBC7SubResourceReservation. To do that, add padding to the required buffer size so that
			// the sub-allocation can be properly aligned.

			if constexpr (Util::Math::IsPowerOfTwo(REQUIRED_BUFFER_BC7_SUB_ALLOCATION_ALIGNMENT))
				Util::Math::AlignToPowerOfTwo(requiredReadBackBufferSize, REQUIRED_BUFFER_BC7_SUB_ALLOCATION_ALIGNMENT);

			else
				Util::Math::Align(requiredReadBackBufferSize, REQUIRED_BUFFER_BC7_SUB_ALLOCATION_ALIGNMENT);

			requiredReadBackBufferSize += hBC7SubResourceReservation->GetReservationSize();
		}

		D3D12::BufferResourceInitializationInfo readBackBufferInitInfo{
			.SizeInBytes = requiredReadBackBufferSize,
			.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK
		};

		mOutputBuffer = std::make_unique<D3D12::BufferResource>(readBackBufferInitInfo);

		struct BC7DataCopyInfo
		{
			D3D12::StructuredBufferSubAllocation<BufferBC7>& DestSubAllocation;
			D3D12::StructuredBufferSubAllocation<BufferBC7> SrcSubAllocation;
		};

		for (auto&& hBC7SubResourceReservation : context.HBC7TextureDataReservationArr)
		{
			const std::size_t numBC7BlocksInReservation = (hBC7SubResourceReservation->GetReservationSize() / sizeof(BufferBC7));
			
			std::optional<D3D12::StructuredBufferSubAllocation<BufferBC7>> bc7BufferSubAllocation{ mOutputBuffer->CreateBufferSubAllocation<D3D12::StructuredBufferSubAllocation<BufferBC7>>(numBC7BlocksInReservation) };
			assert(bc7BufferSubAllocation.has_value());

			mBC7BufferSubAllocationArr.push_back(std::move(*bc7BufferSubAllocation));

			D3D12::RenderPass<D3D12::GPUCommandQueueType::DIRECT, BC7DataCopyInfo> bc7SubResourceCopyPass{};
			bc7SubResourceCopyPass.SetRenderPassName("Opaque Diffuse Model Texture Resolver - Read-Back Buffer BC7 Texture Sub-Resource Copy");

			BC7DataCopyInfo copyInfo{
				.DestSubAllocation{mBC7BufferSubAllocationArr.back()},
				.SrcSubAllocation{ numBC7BlocksInReservation }
			};

			assert(copyInfo.SrcSubAllocation.IsReservationCompatible(hBC7SubResourceReservation));
			copyInfo.SrcSubAllocation.AssignReservation(std::move(hBC7SubResourceReservation));

			bc7SubResourceCopyPass.AddResourceDependency(copyInfo.DestSubAllocation, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
			bc7SubResourceCopyPass.AddResourceDependency(copyInfo.SrcSubAllocation, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE);

			bc7SubResourceCopyPass.SetInputData(std::move(copyInfo));

			bc7SubResourceCopyPass.SetRenderPassCommands([] (D3D12::DirectContext& context, const BC7DataCopyInfo& copyInfo)
			{
				const D3D12::StructuredBufferSnapshot<BufferBC7> destSnapshot{ copyInfo.DestSubAllocation };
				const D3D12::StructuredBufferSnapshot<BufferBC7> srcSnapshot{ copyInfo.SrcSubAllocation };
				
				context.CopyBufferToBuffer(destSnapshot, srcSnapshot);
			});

			D3D12::RenderPassBundle bc7SubResourceCopyBundle{};
			bc7SubResourceCopyBundle.AddRenderPass(std::move(bc7SubResourceCopyPass));

			context.Builder.AddRenderPassBundle(std::move(bc7SubResourceCopyBundle));
		}

		/*
		std::size_t requiredSizeForTextureCopy = 0;

		const Brawler::D3D12_RESOURCE_DESC& mipMappedTextureDesc{ context.CurrTexturePtr->GetResourceDescription() };
		const std::size_t numMipLevels = mipMappedTextureDesc.MipLevels;

		Util::Engine::GetD3D12Device().GetCopyableFootprints1(
			&mipMappedTextureDesc,
			0,
			static_cast<std::uint32_t>(numMipLevels),
			0,
			nullptr,
			nullptr,
			nullptr,
			&requiredSizeForTextureCopy
		);

		const D3D12::BufferResourceInitializationInfo readbackBufferInfo{
			.SizeInBytes = requiredSizeForTextureCopy,
			.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK
		};

		// By creating a resource without using a FrameGraphBuilder, the resource is automatically
		// considered to be persistent.
		mOutputBuffer = std::make_unique<D3D12::BufferResource>(readbackBufferInfo);

		// However, even persistent resources have their ID3D12Resource asynchronously created during
		// FrameGraph generation, so we do not have to do this manually. This is possible because the
		// FrameGraphBuilder knows which resources are being used, as well as which resources do not
		// already have an ID3D12Resource. This applies for both persistent and transient resources.

		mTextureCopySubAllocationArr.reserve(numMipLevels);

		for (auto i : std::views::iota(0u, numMipLevels))
		{
			D3D12::Texture2DSubResource currMipMappedSubResource{ context.CurrTexturePtr->GetSubResource(i) };

			std::optional<D3D12::TextureCopyBufferSubAllocation> subResourceCopySubAllocation{ mOutputBuffer->CreateBufferSubAllocation<D3D12::TextureCopyBufferSubAllocation>(currMipMappedSubResource) };
			assert(subResourceCopySubAllocation.has_value());

			mTextureCopySubAllocationArr.push_back(std::move(*subResourceCopySubAllocation));

			struct TextureCopyInfo
			{
				D3D12::TextureCopyBufferSubAllocation& DestReadbackBufferSubAllocation;
				D3D12::Texture2DSubResource SrcTextureSubResource;
			};

			// We could put all of the sub-resource copies into a single pass containing a dependency
			// for every sub-resource and the read-back BufferResource. However, splitting the process
			// into multiple RenderPassBundles has a few advantages:
			//
			//   - The ability to alias transient resources is done on a per-bundle basis. The Brawler
			//     Engine does not actually alias sub-resources, though, so this essentially has no
			//     impact on this case. However, it can impact other cases.
			//
			//   - We allow for better split barrier generation by using multiple RenderPass instances.
			//
			// Internally, the Brawler Engine will combine the commands of multiple RenderPass instances
			// into a single command list when it deems it safe/possible to do so, so there is no need
			// to worry about a loss in GPU efficiency. In fact, using more RenderPass instances tends
			// to *benefit* GPU efficiency for the aforementioned reasons.

			D3D12::RenderPass<D3D12::GPUCommandQueueType::DIRECT, TextureCopyInfo> readbackCopyPass{};
			readbackCopyPass.SetRenderPassName("Opaque Diffuse Model Texture Resolver - Read-Back Buffer Texture Copy");

			TextureCopyInfo currCopyInfo{
				.DestReadbackBufferSubAllocation{mTextureCopySubAllocationArr.back()},
				.SrcTextureSubResource{ std::move(currMipMappedSubResource) }
			};

			readbackCopyPass.AddResourceDependency(currCopyInfo.DestReadbackBufferSubAllocation, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
			readbackCopyPass.AddResourceDependency(currCopyInfo.SrcTextureSubResource, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE);

			readbackCopyPass.SetInputData(std::move(currCopyInfo));

			readbackCopyPass.SetRenderPassCommands([] (D3D12::DirectContext& context, const TextureCopyInfo& copyInfo)
			{
				context.CopyTextureToBuffer(copyInfo.DestReadbackBufferSubAllocation, copyInfo.SrcTextureSubResource);
			});

			D3D12::RenderPassBundle readbackCopyBundle{};
			readbackCopyBundle.AddRenderPass(std::move(readbackCopyPass));

			context.Builder.AddRenderPassBundle(std::move(readbackCopyBundle));
		}
		*/
	}
}