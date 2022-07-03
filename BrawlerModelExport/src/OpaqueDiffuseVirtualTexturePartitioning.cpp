module;
#include <assimp/material.h>
#include <DxDef.h>

module Brawler.OpaqueDiffuseModelTextureResolver;
import Brawler.VirtualTexturePartitioner;
import Brawler.TextureTypeMap;
import Brawler.ModelTextureID;

namespace Brawler
{
	void OpaqueDiffuseModelTextureResolver::AddVirtualTexturePartitioningRenderPasses(TextureResolutionContext& context)
	{
		// Since virtual texture partitioning does not do anything with the values stored within
		// textures outside of moving them into different textures, we can safely do a
		// ReinterpretResourceCast from sRGB formats to their corresponding non-sRGB format without
		// affecting the correctness of the output. This prevents the hardware from doing any
		// automatic color space conversions, and saves us from having to convert the data back to
		// the sRGB space before writing it out to separate tiles.

		static_assert(Brawler::GetIntermediateTextureFormat<aiTextureType::aiTextureType_DIFFUSE>() == DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);

		AnisotropicFilterVirtualTexturePartitioner<DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM> opaqueDiffuseVirtualTexturePartitioner{ *(context.CurrTexturePtr) };
		opaqueDiffuseVirtualTexturePartitioner.PartitionVirtualTexture(context.Builder);

		context.VirtualTexturePageArr = opaqueDiffuseVirtualTexturePartitioner.ExtractVirtualTexturePages();
	}
}