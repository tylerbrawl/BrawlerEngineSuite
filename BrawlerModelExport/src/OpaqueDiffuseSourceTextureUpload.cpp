module;
#include <cassert>
#include <optional>
#include <span>
#include <ranges>
#include <assimp/material.h>
#include <DxDef.h>
#include <DirectXTex.h>

module Brawler.OpaqueDiffuseModelTextureResolver;
import Util.Math;
import Util.Engine;
import Util.General;
import Brawler.D3D12.Texture2D;
import Brawler.D3D12.Texture2DBuilders;
import Brawler.TextureTypeMap;
import Brawler.D3D12.TextureCopyBufferSubAllocation;
import Brawler.D3D12.FrameGraphBuilding;
import Brawler.D3D12.BufferResourceInitializationInfo;

namespace Brawler
{
	void OpaqueDiffuseModelTextureResolver::AddSourceTextureUploadRenderPasses(TextureResolutionContext& context)
	{
		// STEP 1
		//
		// Copy the DirectX::ScratchImage texture into a transient BufferResource in an UPLOAD heap.
		// Then, copy these contents into a transient Texture2D.

		const DirectX::Image* relevantImagePtr{ mSrcDiffuseScratchImage.GetImage(0, 0, 0) };
		assert(relevantImagePtr != nullptr);

		// Make sure that the image is a power-of-two texture. We can't* do proper mip-mapping if
		// it is not.
		//
		// *The MiniEngine does show a way to do mip-mapping and avoid undersampling. However, this
		// type of mip-mapping/downsampling is much more expensive, as you need to perform multiple
		// samples per texel of the input texture in order to not undersample.
		if (!Util::Math::IsPowerOfTwo(relevantImagePtr->width) || !Util::Math::IsPowerOfTwo(relevantImagePtr->height)) [[unlikely]]
			throw std::runtime_error{ "ERROR: A non-power-of-two-sized texture was detected when performing diffuse model texture resolution for opaque materials!" };

		// We should also make sure that we are dealing with a square texture.
		if (relevantImagePtr->width != relevantImagePtr->height) [[unlikely]]
			throw std::runtime_error{ "ERROR: A non-square texture was detected when performing diffuse model texture resolution for opaque materials!" };

		D3D12::Texture2DBuilder srcTextureBuilder{};
		srcTextureBuilder.SetTextureDimensions(relevantImagePtr->width, relevantImagePtr->height);
		srcTextureBuilder.SetInitialResourceState(D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);

		// We are going to be writing the texture data into buffers during BC7 image compression.
		// If we create the texture with an _SRGB format, then shaders will convert the sRGB data
		// into linear data when we read from the texture. This is indeed the expected behavior.
		//
		// However, this causes us to write *linear* color data into the buffers. Since these are
		// buffers, the shader will *NOT* convert these values back into sRGB space. This causes the
		// data to always be written out in linear space, which is a problem if the destination format
		// is supposed to be an sRGB format.
		//
		// I believe this is why DirectXTex disables hardware color space conversion when doing BC7
		// image compression on the GPU. I could be wrong about this, though.
		static_assert(Brawler::GetIntermediateTextureFormat<aiTextureType::aiTextureType_DIFFUSE>() == DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, "ERROR: The current OpaqueDiffuseModelTextureResolver implementation expects the intermediate format of aiTextureType_DIFFUSE textures to be DXGI_FORMAT_R8G8B8A8_UNORM_SRGB!");
		srcTextureBuilder.SetTextureFormat(DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM);

		D3D12::Texture2D& srcTextureResource{ context.Builder.CreateTransientResource<D3D12::Texture2D>(srcTextureBuilder) };
		D3D12::Texture2DSubResource mainSrcTextureSubResource{ srcTextureResource.GetSubResource() };

		std::uint64_t requiredSizeForTextureCopy = 0;

		Util::Engine::GetD3D12Device().GetCopyableFootprints1(
			&(srcTextureResource.GetResourceDescription()),
			mainSrcTextureSubResource.GetSubResourceIndex(),
			1,
			0,
			nullptr,
			nullptr,
			nullptr,
			&requiredSizeForTextureCopy
		);

		// Create a transient UPLOAD BufferResource for copying the data into the texture.
		D3D12::BufferResource& textureUploadBuffer{ context.Builder.CreateTransientResource<D3D12::BufferResource>(D3D12::BufferResourceInitializationInfo{
			.SizeInBytes = requiredSizeForTextureCopy,
			.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD
		}) };

		D3D12::TextureCopyBufferSubAllocation textureCopySubAllocation{};

		{
			std::optional<D3D12::TextureCopyBufferSubAllocation> optionalSubAllocation{ textureUploadBuffer.CreateBufferSubAllocation<D3D12::TextureCopyBufferSubAllocation>(mainSrcTextureSubResource) };
			assert(optionalSubAllocation.has_value());

			textureCopySubAllocation = std::move(*optionalSubAllocation);
		}

		{
			struct TextureCopyInfo
			{
				D3D12::TextureCopyBufferSubAllocation TextureCopySubAllocation;
				D3D12::Texture2DSubResource DestTextureSubResource;
				const DirectX::Image& SrcImage;
			};

			D3D12::RenderPass<D3D12::GPUCommandQueueType::DIRECT, TextureCopyInfo> textureCopyPass{};
			textureCopyPass.SetRenderPassName("Opaque Diffuse Model Texture Resolver - Source Texture Copy");

			// Adding resource dependencies for resources in UPLOAD or READBACK heaps in technically unnecessary,
			// since it is an error to transition these resources out of their starting state.
			textureCopyPass.AddResourceDependency(textureCopySubAllocation, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE);

			// This one, however, is mandatory.
			textureCopyPass.AddResourceDependency(mainSrcTextureSubResource, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);

			textureCopyPass.SetInputData(TextureCopyInfo{
				.TextureCopySubAllocation{ std::move(textureCopySubAllocation) },
				.DestTextureSubResource{ std::move(mainSrcTextureSubResource) },
				.SrcImage{ *relevantImagePtr }
				});

			textureCopyPass.SetRenderPassCommands([] (D3D12::DirectContext& context, const TextureCopyInfo& copyInfo)
			{
				std::size_t rowPitch = 0;
				std::size_t slicePitch = 0;

				Util::General::CheckHRESULT(DirectX::ComputePitch(
					copyInfo.SrcImage.format,
					copyInfo.SrcImage.width,
					copyInfo.SrcImage.height,
					rowPitch,
					slicePitch
				));

				// Copy the data into the UPLOAD BufferResource. Although the Brawler Engine does support writing
				// into a temporary CPU buffer before ID3D12Resource creation, there is no real point in doing so
				// in this case. When this function is called, however, it is guaranteed that the ID3D12Resource
				// has been created, so we will write directly to the GPU memory.

				const std::size_t rowCount = copyInfo.SrcImage.height;

				for (auto currRow : std::views::iota(0u, rowCount))
				{
					const std::span<const std::uint8_t> currRowDataSpan{ (copyInfo.SrcImage.pixels + (rowPitch * currRow)), rowPitch };
					copyInfo.TextureCopySubAllocation.WriteTextureData(currRow, currRowDataSpan);
				}

				// Copy the data from the UPLOAD BufferResource to the GPU texture.
				context.CopyBufferToTexture(copyInfo.DestTextureSubResource, copyInfo.TextureCopySubAllocation);
			});

			// We actually want our RenderPassBundles to be as small as possible, since this will allow for increased
			// transient resource aliasing.
			D3D12::RenderPassBundle textureCopyBundle{};
			textureCopyBundle.AddRenderPass(std::move(textureCopyPass));

			context.Builder.AddRenderPassBundle(std::move(textureCopyBundle));
		}

		context.CurrTexturePtr = &srcTextureResource;
	}
}