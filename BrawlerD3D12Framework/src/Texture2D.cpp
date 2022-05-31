module;
#include <cassert>
#include "DxDef.h"

module Brawler.D3D12.Texture2D;
import Brawler.D3D12.GPUResourceInitializationInfo;

namespace
{
	Brawler::D3D12::GPUResourceInitializationInfo CreateInitializationInfoFromBuilder(const Brawler::D3D12::Texture2DBuilder& builder)
	{
		return Brawler::D3D12::GPUResourceInitializationInfo{
			.ResourceDesc{ builder.GetResourceDescription() },
			.InitialResourceState = builder.GetInitialResourceState(),

			// Textures must be placed into a D3D12_HEAP_TYPE_DEFAULT heap.
			.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT
		};
	}
}

namespace Brawler
{
	namespace D3D12
	{
		Texture2D::Texture2D(const Texture2DBuilder& builder) :
			I_GPUResource(CreateInitializationInfoFromBuilder(builder))
		{}

		TextureSubResource Texture2D::GetSubResource(const std::uint32_t mipSlice)
		{
			assert(D3D12GetFormatPlaneCount(&(Util::Engine::GetD3D12Device()), GetResourceDescription().Format) == 1 && "Congratulations! You found a multi-planar texture format which isn't meant for depth/stencil textures. Have fun figuring out how to create descriptors for it~");
			
			const std::uint32_t subResourceIndex = D3D12CalcSubresource(
				mipSlice,
				0,
				0,
				static_cast<std::uint32_t>(GetResourceDescription().MipLevels),
				1
			);
			return TextureSubResource{ *this, subResourceIndex };
		}
	}
}