module;
#include <cassert>
#include <optional>
#include "DxDef.h"

module Brawler.D3D12.DepthStencilTexture;
import Brawler.D3D12.GPUResourceInitializationInfo;

namespace
{
	Brawler::D3D12::GPUResourceInitializationInfo CreateGPUResourceInitializationInfo(const Brawler::D3D12::DepthStencilTextureBuilder& builder)
	{
		return Brawler::D3D12::GPUResourceInitializationInfo{
			.ResourceDesc{ builder.GetResourceDescription() },
			.InitialResourceState = builder.GetInitialResourceState(),
			.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT
		};
	}
}

namespace Brawler
{
	namespace D3D12
	{
		template <DepthStencilPlaneID PlaneID>
		DepthStencilTextureSubResource<PlaneID>::DepthStencilTextureSubResource(DepthStencilTexture& depthStencilTexture, const std::uint32_t subResourceIndex) :
			TextureSubResource(subResourceIndex),
			mTexturePtr(&depthStencilTexture)
		{}

		template <DepthStencilPlaneID PlaneID>
		I_GPUResource& DepthStencilTextureSubResource<PlaneID>::GetGPUResource()
		{
			assert(mTexturePtr != nullptr && "ERROR: A DepthStencilTextureSubResource instance was never assigned a DepthStencilTexture instance!");
			return *mTexturePtr;
		}

		template <DepthStencilPlaneID PlaneID>
		const I_GPUResource& DepthStencilTextureSubResource<PlaneID>::GetGPUResource() const
		{
			assert(mTexturePtr != nullptr && "ERROR: A DepthStencilTextureSubResource instance was never assigned a DepthStencilTexture instance!");
			return *mTexturePtr;
		}

		template <DepthStencilPlaneID PlaneID>
		DepthStencilTexture& DepthStencilTextureSubResource<PlaneID>::GetDepthStencilTexture()
		{
			assert(mTexturePtr != nullptr && "ERROR: A DepthStencilTextureSubResource instance was never assigned a DepthStencilTexture instance!");
			return *mTexturePtr;
		}

		template <DepthStencilPlaneID PlaneID>
		const DepthStencilTexture& DepthStencilTextureSubResource<PlaneID>::GetDepthStencilTexture() const
		{
			assert(mTexturePtr != nullptr && "ERROR: A DepthStencilTextureSubResource instance was never assigned a DepthStencilTexture instance!");
			return *mTexturePtr;
		}

		template <DepthStencilPlaneID PlaneID>
		bool DepthStencilTextureSubResource<PlaneID>::CanCreateShaderResourceViews() const
		{
			return ((GetDepthStencilTexture().GetResourceDescription().Flags & D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0);
		}

		template <DepthStencilPlaneID PlaneID>
		std::uint32_t DepthStencilTextureSubResource<PlaneID>::GetMipSlice() const
		{
			const std::uint32_t subResourceIndex = GetSubResourceIndex();

			if constexpr (PlaneID == DepthStencilPlaneID::DEPTH)
				return subResourceIndex;

			else
			{
				static constexpr std::uint32_t PLANE_SLICE = std::to_underlying(PlaneID);

				const Brawler::D3D12_RESOURCE_DESC& resourceDesc{ GetDepthStencilTexture().GetResourceDescription() };

				std::uint32_t mipSlice = 0;
				std::uint32_t arraySlice = 0;
				std::uint32_t planeSlice = 0;

				D3D12DecomposeSubresource(
					subResourceIndex,
					resourceDesc.MipLevels,
					resourceDesc.DepthOrArraySize,
					mipSlice,
					arraySlice,
					planeSlice
				);

				assert(planeSlice == PLANE_SLICE);
				return mipSlice;
			}
		}
	}
}

namespace Brawler
{
	namespace D3D12
	{
		DepthStencilTexture::DepthStencilTexture(const DepthStencilTextureBuilder& builder) :
			I_GPUResource(CreateGPUResourceInitializationInfo(builder)),
			mOptimizedClearValue(builder.GetOptimizedClearValue()),
			mInitMethod(builder.GetPreferredSpecialInitializationMethod())
		{}

		std::optional<D3D12_CLEAR_VALUE> DepthStencilTexture::GetOptimizedClearValue() const
		{
			return mOptimizedClearValue;
		}

		GPUResourceSpecialInitializationMethod DepthStencilTexture::GetPreferredSpecialInitializationMethod() const
		{
			return mInitMethod;
		}

		DepthTextureSubResource DepthStencilTexture::GetDepthTextureSubResource(const std::uint32_t mipSlice)
		{
			return DepthTextureSubResource{ *this, mipSlice };
		}

		StencilTextureSubResource DepthStencilTexture::GetStencilTextureSubResource(const std::uint32_t mipSlice)
		{
			static constexpr std::uint32_t STENCIL_PLANE_SLICE = std::to_underlying(DepthStencilPlaneID::STENCIL);
			
			const Brawler::D3D12_RESOURCE_DESC& resourceDesc{ GetResourceDescription() };
			const std::uint32_t subResourceIndex = D3D12CalcSubresource(
				mipSlice,
				0,
				STENCIL_PLANE_SLICE,
				resourceDesc.MipLevels,
				resourceDesc.DepthOrArraySize
			);

			return StencilTextureSubResource{ *this, subResourceIndex };
		}

		bool DepthStencilTexture::CanCreateShaderResourceViews() const
		{
			return ((GetResourceDescription().Flags & D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0);
		}
	}
}

namespace Brawler
{
	namespace D3D12
	{
		template class DepthStencilTextureSubResource<DepthStencilPlaneID::DEPTH>;
		template class DepthStencilTextureSubResource<DepthStencilPlaneID::STENCIL>;
	}
}