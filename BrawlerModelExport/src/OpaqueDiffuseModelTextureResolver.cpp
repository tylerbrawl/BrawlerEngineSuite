module;
#include <optional>
#include <cassert>
#include <stdexcept>
#include <format>
#include <memory>
#include <span>
#include <ranges>
#include <assimp/material.h>
#include <DxDef.h>
#include <DirectXTex.h>

module Brawler.OpaqueDiffuseModelTextureResolver;
import Brawler.AssimpTextureConverter;
import Brawler.AssimpMaterials;
import Brawler.D3D12.BufferResourceInitializationInfo;
import Util.Engine;
import Util.Math;
import Util.General;
import Brawler.D3D12.Renderer;
import Brawler.ModelTextureResolutionRenderModule;
import Brawler.D3D12.Texture2D;
import Brawler.D3D12.Texture2DBuilders;
import Brawler.D3D12.TextureSubResource;
import Brawler.TextureTypeMap;
import Brawler.D3D12.FrameGraphBuilding;
import Brawler.D3D12.BC7ImageCompressor;
import Brawler.D3D12.BufferSubAllocationReservation;

namespace Brawler
{
	extern D3D12::Renderer& GetRenderer();
}

namespace Brawler
{
	OpaqueDiffuseModelTextureResolver::OpaqueDiffuseModelTextureResolver(const ImportedMesh& mesh) :
		mHDiffuseTextureResolutionEvent(),
		mOutputBuffer(nullptr),
		mSrcDiffuseScratchImage(),
		mMeshPtr(std::addressof(mesh))
	{}

	void OpaqueDiffuseModelTextureResolver::Update()
	{
		if (!mHDiffuseTextureResolutionEvent.has_value())
			BeginDiffuseTextureResolution();
	}

	void OpaqueDiffuseModelTextureResolver::BeginDiffuseTextureResolution()
	{
		// The idea behind model texture resolution is that different materials are going to do different
		// things with textures which would normally go into the same material slot. For instance, an
		// opaque material has no transparency, so it would be a waste to have the alpha channel of its
		// diffuse texture represent that. Instead, it can be used to denote a different value, such as
		// whether or not the object is metallic.
		//
		// The process is as follows:
		//
		//   1. Construct an AssimpTextureConverter for each texture type which is to be taken from the
		//      model in order to construct the final texture. For instance, one might take a normal map
		//      and a roughness texture from the model for combination later on. It might also be the case
		//      that the same AssimpTextureConverter instance is used to get multiple of the same texture
		//      type, e.g., for multi-layered materials.
		//
		//   2. Pass a callback function to the ModelTextureResolutionRenderModule which takes a
		//      D3D12::FrameGraphBuilder& and applies all of the necessary GPU tasks to the 
		//      DirectX::ScratchImage instances created from the AssimpTextureConverter instances. This can
		//      be a multi-step process, but all applicable tasks/RenderPasses should be moved into the 
		//      D3D12::FrameGraphBuilder& provided to the callback.
		//
		//   3. Wait for the ModelTextureResolutionEventHandle instance returned by the
		//      ModelTextureResolutionRenderModule to inform us that all of the steps have been completed
		//      on the GPU timeline. This can be determined by calling 
		//      ModelTextureResolutionEventHandle::IsEventComplete().
		//
		// For this simple model texture resolver, we are going to take one diffuse texture from the
		// provided ImportedMesh instance using a single AssimpTextureConverter. Then, we are going to perform
		// the following processes on the GPU timeline in order:
		//
		//   - Copy the DirectX::ScratchImage texture into a transient BufferResource in an UPLOAD heap. (Yes, 
		//     we can copy it into a transient one. We just need to make sure that the DirectX::ScratchImage 
		//     instances outlive the task on the GPU timeline.) Then, copy these contents into a transient
		//     Texture2D.
		//
		//   - Convert the data contained in the Texture2D to the BC7 format.
		//
		//   - Copy the converted buffer texture data to a transient Texture2D resource and perform generic
		//     GPU-based mip-mapping of the texture, down to a 1x1 version. We do this with our own shader,
		//     rather than with DirectXTex, because DirectXTex's mip-mapping function does not support block-
		//     compressed formats (and because I desperately need HLSL practice).
		//
		//   - Copy the generated textures into a persistent BufferResource in a READBACK heap. (Yes, this one
		//     must be persistent. How else are we going to get the data back?)
		//
		// After all of those steps have been completed on the GPU timeline, 
		// ModelTextureResolutionEventHandle::IsEventComplete() will return true, and we can copy from the
		// persistent BufferResource to another DirectX::ScratchImage instance.

		assert(mMeshPtr != nullptr);

		AssimpTextureConverter<aiTextureType::aiTextureType_DIFFUSE> diffuseTextureConverter{};
		diffuseTextureConverter.BeginTextureConversion(*mMeshPtr);

		if (!diffuseTextureConverter.HasConvertedTextures()) [[unlikely]]
		{
			const std::optional<aiString> materialName{ Brawler::GetAssimpMaterialProperty<AssimpMaterialKeyID::NAME>(mMeshPtr->GetMeshMaterial()) };

			if (materialName.has_value()) [[likely]]
				throw std::runtime_error{ std::format("ERROR: An attempt was made to resolve an opaque diffuse texture for the material {}, but no opaque texture could be found/created!", materialName->C_Str()) };
			else [[unlikely]]
				throw std::runtime_error{ "ERROR: An attempt was made to resolve an opaque diffuse texture for an unnamed material, but no opaque texture could be found/created!" };
		}

		const std::span<DirectX::ScratchImage> srcScratchImageSpan{ diffuseTextureConverter.GetConvertedTextureSpan() };
		assert(!srcScratchImageSpan.empty());

		DirectX::ScratchImage& relevantSrcScratchImage{ srcScratchImageSpan[0] };
		assert(relevantSrcScratchImage.GetImageCount() > 0);

		mSrcDiffuseScratchImage = std::move(relevantSrcScratchImage);

		ModelTextureResolutionRenderModule& textureResolutionModule{ GetRenderer().GetRenderModule<ModelTextureResolutionRenderModule>() };
		mHDiffuseTextureResolutionEvent = textureResolutionModule.RegisterTextureResolutionCallback([this] (D3D12::FrameGraphBuilder& builder)
		{
			AddTextureResolutionRenderPasses(builder);
		});
	}

	void OpaqueDiffuseModelTextureResolver::AddTextureResolutionRenderPasses(D3D12::FrameGraphBuilder& builder)
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

		D3D12::Texture2D& srcTextureResource{ builder.CreateTransientResource<D3D12::Texture2D>(srcTextureBuilder) };
		D3D12::TextureSubResource mainSrcTextureSubResource{ srcTextureResource.GetSubResource() };

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
		D3D12::BufferResource& textureUploadBuffer{ builder.CreateTransientResource<D3D12::BufferResource>(D3D12::BufferResourceInitializationInfo{
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
				D3D12::TextureSubResource DestTextureSubResource;
				const DirectX::Image& SrcImage;
			};

			D3D12::RenderPass<GPUCommandQueueType::DIRECT, TextureCopyInfo> textureCopyPass{};
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

			builder.AddRenderPassBundle(std::move(textureCopyBundle));
		}

		// STEP 2
		//
		// Convert the data contained in the Texture2D to the BC7 format.
		D3D12::BufferSubAllocationReservation bc7TextureDataReservation{ [&builder, &srcTextureResource]()
		{
			BC7ImageCompressor bc7Compressor{BC7ImageCompressor::InitInfo{
				.SrcTextureSubResource{srcTextureResource.GetSubResource()},
				.DesiredFormat = Brawler::GetDesiredTextureFormat<aiTextureType::aiTextureType_DIFFUSE>()
			} };

			return bc7Compressor.AddCompressionRenderPasses(builder);
		}() };

		// STEP 3
		//
		// Copy the converted buffer texture data to a transient Texture2D resource and perform generic
		// GPU-based mip-mapping of the texture, down to a 1x1 version.

		// First, we need to calculate how many mip levels our texture is going to need.
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
		mipMappedTextureBuilder.SetMipLevelCount(numMipLevels);
		mipMappedTextureBuilder.SetTextureFormat(Brawler::GetDesiredTextureFormat<aiTextureType::aiTextureType_DIFFUSE>());
		mipMappedTextureBuilder.SetInitialResourceState(D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);

		// We'll need to allow UAVs, since we will be writing to it during the mip-mapping process.
		mipMappedTextureBuilder.AllowUnorderedAccessViews();

		D3D12::Texture2D& mipMappedTextureResource{ builder.CreateTransientResource<D3D12::Texture2D>(mipMappedTextureBuilder) };

		// Copy the converted BC7 texture data into the highest mip level of the Texture2D which we just
		// created.
		{
			struct TextureCopyInfo
			{
				D3D12::TextureSubResource DestTextureSubResource;
				D3D12::TextureCopyBufferSubAllocation SrcTextureCopySubAllocation;
			};

			D3D12::RenderPass<GPUCommandQueueType::DIRECT, TextureCopyInfo> bc7TextureDataCopyPass{};
			bc7TextureDataCopyPass.SetRenderPassName("Opaque Diffuse Model Texture Resolver - BC7 Texture Data Copy");

			D3D12::TextureSubResource highestMipDestSubResource{ mipMappedTextureResource.GetSubResource(0) };
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
			assert(bc7TextureCopySubAllocation.IsReservationCompatible(bc7TextureDataReservation));
			bc7TextureCopySubAllocation.AssignReservation(std::move(bc7TextureDataReservation));

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
			bc7TextureDataCopyBundle.AddRenderPass(std::move(bc7TextureDataCopyBundle));

			builder.AddRenderPassBundle(std::move(bc7TextureDataCopyBundle));
		}
	}
}