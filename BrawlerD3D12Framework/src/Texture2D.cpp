module;
#include <cassert>
#include <optional>
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
			I_GPUResource(CreateInitializationInfoFromBuilder(builder)),
			mOptimizedClearValue(builder.GetOptimizedClearValue()),
			mInitMethod(builder.GetPreferredSpecialInitializationMethod())
		{}

		std::optional<D3D12_CLEAR_VALUE> Texture2D::GetOptimizedClearValue() const
		{
			return mOptimizedClearValue;
		}

		GPUResourceSpecialInitializationMethod Texture2D::GetPreferredSpecialInitializationMethod() const
		{
			return mInitMethod;
		}

		Texture2DSubResource Texture2D::GetSubResource(const std::uint32_t mipSlice)
		{
			assert(D3D12GetFormatPlaneCount(&(Util::Engine::GetD3D12Device()), GetResourceDescription().Format) == 1 && "Congratulations! You found a multi-planar texture format which isn't meant for depth/stencil textures. Have fun figuring out how to create descriptors for it~");
			
			const std::uint32_t subResourceIndex = D3D12CalcSubresource(
				mipSlice,
				0,
				0,
				static_cast<std::uint32_t>(GetResourceDescription().MipLevels),
				1
			);
			return Texture2DSubResource{ *this, subResourceIndex };
		}
	}
}

namespace Brawler
{
	namespace D3D12
	{
		Texture2DSubResource::Texture2DSubResource(Texture2D& texture2D, const std::uint32_t subResourceIndex) :
			TextureSubResource(subResourceIndex),
			mTexturePtr(&texture2D)
		{}

		I_GPUResource& Texture2DSubResource::GetGPUResource()
		{
			assert(mTexturePtr != nullptr);
			return *mTexturePtr;
		}

		const I_GPUResource& Texture2DSubResource::GetGPUResource() const
		{
			assert(mTexturePtr != nullptr);
			return *mTexturePtr;
		}

		Texture2D& Texture2DSubResource::GetTexture2D()
		{
			assert(mTexturePtr != nullptr);
			return *mTexturePtr;
		}

		const Texture2D& Texture2DSubResource::GetTexture2D() const
		{
			assert(mTexturePtr != nullptr);
			return *mTexturePtr;
		}
	}
}