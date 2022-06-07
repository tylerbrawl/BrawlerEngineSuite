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
import Util.General;
import Brawler.Application;
import Brawler.D3D12.Renderer;
import Brawler.ModelTextureResolutionRenderModule;
import Brawler.D3D12.Texture2D;
import Brawler.TextureTypeMap;

namespace Brawler
{
	OpaqueDiffuseModelTextureResolver::OpaqueDiffuseModelTextureResolver(const ImportedMesh& mesh) :
		mHDiffuseTextureResolutionEvent(),
		mOutputBuffer(nullptr),
		mBC7CompressorPtrArr(),
		mBC7BufferSubAllocationArr(),
		mDestDiffuseScratchImage(),
		mSrcDiffuseScratchImage(),
		mMeshPtr(&mesh)

	// We leave the constructor empty and only perform work in the OpaqueDiffuseModelTextureResolver::Update()
	// function in order to take advantage of concurrency, since each I_MaterialDefinition which would
	// ordinarily own one of these is updated in parallel.
	{}

	void OpaqueDiffuseModelTextureResolver::Update()
	{
		if (!mHDiffuseTextureResolutionEvent.has_value()) [[unlikely]]
			BeginDiffuseTextureResolution();

		else if (mHDiffuseTextureResolutionEvent->IsEventComplete() && mOutputBuffer != nullptr)
			CopyResolvedTextureToScratchImage();
	}

	bool OpaqueDiffuseModelTextureResolver::IsReadyForSerialization() const
	{
		return (mHDiffuseTextureResolutionEvent.has_value() && mHDiffuseTextureResolutionEvent->IsEventComplete() && mOutputBuffer == nullptr);
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
		//   - Copy the converted buffer texture data to a transient Texture2D resource and perform generic
		//     GPU-based mip-mapping of the texture, down to a 1x1 version.
		// 
		//   - For each generated mip level of the texture, create the equivalent BC7 version. We use a slightly
		//     modified version of the image compression shader used by DirectXTex. (The shader was modified to
		//     allow for more optimized root parameter updates.)
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

		const std::span<ConvertedAssimpTexture> srcConvertedTextureSpan{ diffuseTextureConverter.GetConvertedTextureSpan() };
		assert(!srcConvertedTextureSpan.empty());

		DirectX::ScratchImage& relevantSrcScratchImage{ srcConvertedTextureSpan[0].ScratchImage };
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
		// Originally, I had all of the tasks implemented in this one file and under this one
		// function. However, I was getting MSVC error C1605 because the resulting object file
		// was getting too bloated for the compiler to handle. So, each step was moved to its
		// own module implementation unit and implemented as a separate member function.
		//
		// While it is in general nicer to have smaller functions like this, the fact that I
		// was even hitting error C1605 kind of worries me. We'll just have to see how things
		// turn out in practice later on.
		//
		// UPDATE: As it turns out, the MSVC bug was caused by the implementation of
		// DescriptorTableBuilder::NullifyUnorderedAccessView(). I'm thinking of filing a bug
		// report. Anyways, we'll leave the steps in separate module implementation units
		// regardless.

		TextureResolutionContext resolutionContext{
			.Builder{builder},
			.CurrTexturePtr = nullptr,
			.HBC7TextureDataReservationArr{}
		};

		AddSourceTextureUploadRenderPasses(resolutionContext);
		AddMipMapGenerationRenderPasses(resolutionContext);
		AddBC7CompressionRenderPasses(resolutionContext);
		AddReadBackBufferCopyRenderPasses(resolutionContext);
	}

	void OpaqueDiffuseModelTextureResolver::CopyResolvedTextureToScratchImage()
	{
		assert(mHDiffuseTextureResolutionEvent->IsEventComplete() && mOutputBuffer != nullptr);

		{
			const DirectX::Image* const srcImagePtr = mSrcDiffuseScratchImage.GetImage(0, 0, 0);
			assert(srcImagePtr != nullptr);

			// Initialize the DirectX::ScratchImage to have one image for every sub-resource of the
			// transient Texture2D which contained our data.
			Util::General::CheckHRESULT(mDestDiffuseScratchImage.Initialize2D(
				Brawler::GetDesiredTextureFormat<aiTextureType::aiTextureType_DIFFUSE>(),
				srcImagePtr->width,
				srcImagePtr->height,
				1,
				mBC7BufferSubAllocationArr.size(),
				DirectX::CP_FLAGS::CP_FLAGS_NONE
			));
		}

		// Copy the data from each StructuredBufferSubAllocation<BufferBC7> (i.e., each sub-resource) to
		// its corresponding DirectX::Image. (Before you ask: No, we still don't have std::views::zip.)
		const std::span<const DirectX::Image> destImageSpan{ mDestDiffuseScratchImage.GetImages(), mDestDiffuseScratchImage.GetImageCount() };
		assert(destImageSpan.size() == mBC7BufferSubAllocationArr.size());

		for (const auto i : std::views::iota(0u, mBC7BufferSubAllocationArr.size()))
		{
			const DirectX::Image& currDestImage{ destImageSpan[i] };
			const D3D12::StructuredBufferSubAllocation<BufferBC7>& currSrcDataSubAllocation{ mBC7BufferSubAllocationArr[i] };

			std::size_t rowPitch = 0;
			std::size_t slicePitch = 0;

			Util::General::CheckHRESULT(DirectX::ComputePitch(
				currDestImage.format,
				currDestImage.width,
				currDestImage.height,
				rowPitch,
				slicePitch
			));

			const std::size_t numRows = (slicePitch / rowPitch);
			assert(numRows == std::max<std::size_t>(1, (currDestImage.height + 3) >> 2));

			const std::size_t numBlocksPerRow = std::max<size_t>(1, (currDestImage.width + 3) >> 2);

			for (std::size_t currRow = 0; currRow < numRows; ++currRow)
			{
				const std::span<BufferBC7> destRowDataSpan{ reinterpret_cast<BufferBC7*>(currDestImage.pixels + (currRow * rowPitch)), numBlocksPerRow };
				currSrcDataSubAllocation.ReadStructuredBufferData(static_cast<std::uint32_t>(currRow * numBlocksPerRow), destRowDataSpan);
			}
		}

		// Delete the persistent BufferResource and all of its associated StructuredBufferSubAllocation
		// instances. Not only does this tell us if we are finished with everything (assuming that
		// mHDiffuseTextureResolutionEvent->IsEventComplete() returns true, of course), but it also
		// frees the GPU memory reserved by the BufferResource.
		mOutputBuffer.reset();
		mBC7BufferSubAllocationArr.clear();
	}
}