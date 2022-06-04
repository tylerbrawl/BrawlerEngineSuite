module;
#include <cassert>
#include <ranges>
#include <DxDef.h>
#include <DirectXTex.h>

module Brawler.OpaqueDiffuseModelTextureResolver;
import Brawler.BC7ImageCompressor;
import Brawler.TextureTypeMap;
import Brawler.D3D12.Texture2D;
import Brawler.D3D12.BufferSubAllocationReservationHandle;

namespace Brawler
{
	void OpaqueDiffuseModelTextureResolver::AddBC7CompressionRenderPasses(TextureResolutionContext& context)
	{
		// STEP 2
		//
		// Convert the data contained in the Texture2D to the BC7 format.
		assert(context.CurrTexturePtr != nullptr);

		const std::uint16_t numMipLevels = context.CurrTexturePtr->GetResourceDescription().MipLevels;

		context.HBC7TextureDataReservationArr.reserve(numMipLevels);
		mBC7CompressorPtrArr.reserve(numMipLevels);

		for (auto i : std::views::iota(0u, numMipLevels))
		{
			std::unique_ptr<BC7ImageCompressor> compressorPtr{ std::make_unique<BC7ImageCompressor>(BC7ImageCompressor::InitInfo{
				.SrcTextureSubResource{ context.CurrTexturePtr->GetSubResource(i) },
				.DesiredFormat = Brawler::GetDesiredTextureFormat<aiTextureType::aiTextureType_DIFFUSE>()
			}) };
			
			D3D12::BufferSubAllocationReservationHandle hBC7SubResourceDataReservation{ compressorPtr->AddCompressionRenderPasses(context.Builder) };

			context.HBC7TextureDataReservationArr.push_back(std::move(hBC7SubResourceDataReservation));
			mBC7CompressorPtrArr.push_back(std::move(compressorPtr));
		}
	}
}