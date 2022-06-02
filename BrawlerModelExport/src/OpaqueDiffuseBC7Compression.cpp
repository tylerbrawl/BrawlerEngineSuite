module;
#include <cassert>
#include <DirectXTex.h>

module Brawler.OpaqueDiffuseModelTextureResolver;
import Brawler.BC7ImageCompressor;
import Brawler.TextureTypeMap;

namespace Brawler
{
	void OpaqueDiffuseModelTextureResolver::AddBC7CompressionRenderPasses(TextureResolutionContext& context)
	{
		// STEP 2
		//
		// Convert the data contained in the Texture2D to the BC7 format.
		assert(context.CurrTexturePtr != nullptr);

		context.HBC7TextureDataReservation = [&context]()
		{
			BC7ImageCompressor bc7Compressor{BC7ImageCompressor::InitInfo{
				.SrcTextureSubResource{ context.CurrTexturePtr->GetSubResource() },
				.DesiredFormat = Brawler::GetDesiredTextureFormat<aiTextureType::aiTextureType_DIFFUSE>()
			} };

			return bc7Compressor.AddCompressionRenderPasses(context.Builder);
		}();
	}
}