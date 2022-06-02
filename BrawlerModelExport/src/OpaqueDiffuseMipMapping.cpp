module;
#include <span>
#include <cassert>
#include <assimp/material.h>
#include <DirectXTex.h>

module Brawler.OpaqueDiffuseModelTextureResolver;
import Brawler.D3D12.Texture2DBuilders;
import Brawler.D3D12.TextureCopyBufferSubAllocation;
import Brawler.D3D12.FrameGraphBuilding;
import Brawler.TextureTypeMap;
import Brawler.MipMapGeneration;

namespace Brawler
{
	void OpaqueDiffuseModelTextureResolver::AddMipMapGenerationRenderPasses(TextureResolutionContext& context)
	{
		// STEP 3
		//
		// Copy the converted buffer texture data to a transient Texture2D resource and perform generic
		// GPU-based mip-mapping of the texture, down to a 1x1 version.

		// First, we need to calculate how many mip levels our texture is going to need.
		const DirectX::Image* const relevantImagePtr{ mSrcDiffuseScratchImage.GetImage(0, 0, 0) };
		assert(relevantImagePtr != nullptr);

		std::size_t numMipLevels = 1;

		{
			std::size_t currWidth = relevantImagePtr->width;

			while (currWidth > 1)
			{
				currWidth /= 2;
				++numMipLevels;
			}
		}

		D3D12::Texture2DBuilder mipMappedTextureBuilder{};
		mipMappedTextureBuilder.SetTextureDimensions(relevantImagePtr->width, relevantImagePtr->height);
		mipMappedTextureBuilder.SetMipLevelCount(static_cast<std::uint16_t>(numMipLevels));
		mipMappedTextureBuilder.SetTextureFormat(Brawler::GetDesiredTextureFormat<aiTextureType::aiTextureType_DIFFUSE>());
		mipMappedTextureBuilder.SetInitialResourceState(D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);

		// We'll need to allow UAVs, since we will be writing to it during the mip-mapping process.
		mipMappedTextureBuilder.AllowUnorderedAccessViews();

		D3D12::Texture2D& mipMappedTextureResource{ context.Builder.CreateTransientResource<D3D12::Texture2D>(mipMappedTextureBuilder) };

		// Copy the converted BC7 texture data into the highest mip level of the Texture2D which we just
		// created.
		{
			struct TextureCopyInfo
			{
				D3D12::Texture2DSubResource DestTextureSubResource;
				D3D12::TextureCopyBufferSubAllocation SrcTextureCopySubAllocation;
			};

			D3D12::RenderPass<D3D12::GPUCommandQueueType::DIRECT, TextureCopyInfo> bc7TextureDataCopyPass{};
			bc7TextureDataCopyPass.SetRenderPassName("Opaque Diffuse Model Texture Resolver - BC7 Texture Data Copy");

			D3D12::Texture2DSubResource highestMipDestSubResource{ mipMappedTextureResource.GetSubResource(0) };
			D3D12::TextureCopyBufferSubAllocation bc7TextureCopySubAllocation{ highestMipDestSubResource };

			// Move the BC7 texture data taken from the BC7ImageCompressor into the bc7TextureCopySubAllocation.
			// This is the Brawler Engine's equivalent to a std::move() of GPU buffer memory.
			//
			// One might wonder how this is possible without first assigning bc7TextureCopySubAllocation a
			// BufferResource. It is important to note that all a buffer sub-allocation represents is a way to
			// use GPU buffer memory. BufferSubAllocationReservation instances represent actual segments of
			// GPU buffer memory.
			//
			// By assigning bc7TextureCopySubAllocation a BufferSubAllocationReservation, we are giving it
			// the segment of GPU memory created by the BC7ImageCompressor; bc7TextureCopySubAllocation now
			// owns this memory.
			assert(bc7TextureCopySubAllocation.IsReservationCompatible(context.HBC7TextureDataReservation));
			bc7TextureCopySubAllocation.AssignReservation(std::move(context.HBC7TextureDataReservation));

			bc7TextureDataCopyPass.AddResourceDependency(highestMipDestSubResource, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
			bc7TextureDataCopyPass.AddResourceDependency(bc7TextureCopySubAllocation, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE);

			bc7TextureDataCopyPass.SetInputData(TextureCopyInfo{
				.DestTextureSubResource{ std::move(highestMipDestSubResource) },
				.SrcTextureCopySubAllocation{ std::move(bc7TextureCopySubAllocation) }
				});

			bc7TextureDataCopyPass.SetRenderPassCommands([] (D3D12::DirectContext& context, const TextureCopyInfo& copyInfo)
			{
				context.CopyBufferToTexture(copyInfo.DestTextureSubResource, copyInfo.SrcTextureCopySubAllocation);
			});

			D3D12::RenderPassBundle bc7TextureDataCopyBundle{};
			bc7TextureDataCopyBundle.AddRenderPass(std::move(bc7TextureDataCopyPass));

			context.Builder.AddRenderPassBundle(std::move(bc7TextureDataCopyBundle));
		}

		// Perform the mip-mapping process.
		{
			GenericMipMapGenerator<Brawler::GetDesiredTextureFormat<aiTextureType::aiTextureType_DIFFUSE>()> mipMapGenerator{ mipMappedTextureResource };
			mipMapGenerator.CreateMipMapGenerationRenderPasses(context.Builder);
		}

		context.CurrTexturePtr = &mipMappedTextureResource;
	}
}