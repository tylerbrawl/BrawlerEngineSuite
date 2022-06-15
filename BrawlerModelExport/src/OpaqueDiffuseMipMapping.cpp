module;
#include <span>
#include <cassert>
#include <assimp/material.h>
#include <DirectXTex.h>

module Brawler.OpaqueDiffuseModelTextureResolver;
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

		// Perform the mip-mapping process.
		assert(context.CurrTexturePtr != nullptr);

		{
			GenericMipMapGenerator<Brawler::GetIntermediateTextureFormat<aiTextureType::aiTextureType_DIFFUSE>()> mipMapGenerator{ *(context.CurrTexturePtr) };
			mipMapGenerator.CreateMipMapGenerationRenderPasses(context.Builder);
		}
	}
}