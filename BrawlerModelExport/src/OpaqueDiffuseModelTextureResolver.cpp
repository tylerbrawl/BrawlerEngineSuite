module;
#include <optional>
#include <cassert>
#include <stdexcept>
#include <format>
#include <assimp/material.h>

module Brawler.OpaqueDiffuseModelTextureResolver;
import Brawler.AssimpTextureConverter;
import Brawler.AssimpMaterials;

namespace Brawler
{
	OpaqueDiffuseModelTextureResolver::OpaqueDiffuseModelTextureResolver(const ImportedMesh& mesh) :
		mHDiffuseTextureResolution(),
		mMeshPtr(std::addressof(mesh))
	{}
	
	void OpaqueDiffuseModelTextureResolver::Update()
	{
		if (!mHDiffuseTextureResolution.has_value())
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
		//     instances outlive the task on the GPU timeline.)
		//
		//   - Convert the data contained in this persistent BufferResource to the BC7 format.
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
		
		
	}
}