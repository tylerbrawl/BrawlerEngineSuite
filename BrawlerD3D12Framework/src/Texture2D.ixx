module;
#include <cstdint>
#include <cassert>
#include <optional>
#include "DxDef.h"

export module Brawler.D3D12.Texture2D;
import Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.Texture2DBuilders;
import Brawler.D3D12.GPUResourceViews;
import Util.Engine;
import Brawler.D3D12.TextureSubResource;
import Brawler.D3D12.GPUResourceSpecialInitializationMethod;

export namespace Brawler
{
	namespace D3D12
	{
		class Texture2D;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class Texture2DSubResource final : public TextureSubResource
		{
		public:
			Texture2DSubResource() = default;
			Texture2DSubResource(Texture2D& texture2D, const std::uint32_t subResourceIndex);

			Texture2DSubResource(const Texture2DSubResource& rhs) = default;
			Texture2DSubResource& operator=(const Texture2DSubResource& rhs) = default;

			Texture2DSubResource(Texture2DSubResource&& rhs) noexcept = default;
			Texture2DSubResource& operator=(Texture2DSubResource&& rhs) noexcept = default;

			I_GPUResource& GetGPUResource() override;
			const I_GPUResource& GetGPUResource() const override;

			Texture2D& GetTexture2D();
			const Texture2D& GetTexture2D() const;

			template <DXGI_FORMAT Format>
			Texture2DShaderResourceView<Format> CreateShaderResourceView() const;

			template <DXGI_FORMAT Format>
			Texture2DUnorderedAccessView<Format> CreateUnorderedAccessView() const;

			template <DXGI_FORMAT Format>
			BindlessSRVAllocation CreateBindlessSRV();

		private:
			Texture2D* mTexturePtr;
		};
	}
}

// --------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <DXGI_FORMAT Format>
		Texture2DShaderResourceView<Format> Texture2DSubResource::CreateShaderResourceView() const
		{
			assert(D3D12GetFormatPlaneCount(&(Util::Engine::GetD3D12Device()), GetResourceDescription().Format) == 1 && "Congratulations! You found a multi-planar texture format which isn't meant for depth/stencil textures. Have fun figuring out how to create descriptors for it~");

			return Texture2DShaderResourceView<Format>{ GetTexture2D(), D3D12_TEX2D_SRV{
				.MostDetailedMip = GetSubResourceIndex(),
				.MipLevels = 1,
				.PlaneSlice = 0,
				.ResourceMinLODClamp = 0.0f
			} };
		}

		template <DXGI_FORMAT Format>
		Texture2DUnorderedAccessView<Format> Texture2DSubResource::CreateUnorderedAccessView() const
		{
			assert(D3D12GetFormatPlaneCount(&(Util::Engine::GetD3D12Device()), GetResourceDescription().Format) == 1 && "Congratulations! You found a multi-planar texture format which isn't meant for depth/stencil textures. Have fun figuring out how to create descriptors for it~");

			return Texture2DUnorderedAccessView<Format>{ GetTexture2D(), D3D12_TEX2D_UAV{
				.MipSlice = GetSubResourceIndex(),
				.PlaneSlice = 0
			} };
		}

		template <DXGI_FORMAT Format>
		BindlessSRVAllocation Texture2DSubResource::CreateBindlessSRV()
		{
			const Texture2DShaderResourceView<Format> tex2DSrv{ CreateShaderResourceView<Format>() };
			I_GPUResource& texture2DResource = static_cast<I_GPUResource&>(GetTexture2D());

			return texture2DResource.CreateBindlessSRV(tex2DSrv.CreateSRVDescription());
		}
	}
}

// --------------------------------------------------------------------------------------------------------------

export namespace Brawler
{
	namespace D3D12
	{
		class Texture2D final : public I_GPUResource
		{
		public:
			explicit Texture2D(const Texture2DBuilder& builder);
			explicit Texture2D(const RenderTargetTexture2DBuilder& builder);

			Texture2D(const Texture2D& rhs) = delete;
			Texture2D& operator=(const Texture2D& rhs) = delete;

			Texture2D(Texture2D&& rhs) noexcept = default;
			Texture2D& operator=(Texture2D&& rhs) noexcept = default;

			template <DXGI_FORMAT Format>
			Texture2DShaderResourceView<Format> CreateShaderResourceView() const;

			template <DXGI_FORMAT Format>
			Texture2DUnorderedAccessView<Format> CreateUnorderedAccessView(const std::uint32_t mipSlice = 0) const;

			std::optional<D3D12_CLEAR_VALUE> GetOptimizedClearValue() const override;
			GPUResourceSpecialInitializationMethod GetPreferredSpecialInitializationMethod() const override;

			/// <summary>
			/// Creates and returns a Texture2DSubResource instance which refers to the mip level
			/// of this Texture2D indicated by mipSlice. This directly corresponds to the idea of
			/// sub-resources from the D3D12 API.
			/// </summary>
			/// <param name="mipSlice">
			/// - The mip level for which a Texture2DSubResource instance will be created. If no
			///   value is specified, then a Texture2DSubResource instance is created for the highest
			///   quality mip level (i.e., mipSlice == 0).
			/// </param>
			/// <returns>
			/// The function returns a Texture2DSubResource instance which refers to the mip level
			/// of this Texture2D indicated by mipSlice.
			/// </returns>
			Texture2DSubResource GetSubResource(const std::uint32_t mipSlice = 0);

			template <DXGI_FORMAT Format>
			BindlessSRVAllocation CreateBindlessSRV();

		private:
			std::optional<D3D12_CLEAR_VALUE> mOptimizedClearValue;
			GPUResourceSpecialInitializationMethod mInitMethod;
		};
	}
}

// --------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <DXGI_FORMAT Format>
		Texture2DShaderResourceView<Format> Texture2D::CreateShaderResourceView() const
		{
			assert(D3D12GetFormatPlaneCount(&(Util::Engine::GetD3D12Device()), GetResourceDescription().Format) == 1 && "Congratulations! You found a multi-planar texture format which isn't meant for depth/stencil textures. Have fun figuring out how to create descriptors for it~");

			return Texture2DShaderResourceView<Format>{ *this, D3D12_TEX2D_SRV{
				.MostDetailedMip = 0,
				.MipLevels = static_cast<std::uint32_t>(-1),
				.PlaneSlice = 0,
				.ResourceMinLODClamp = 0.0f
			} };
		}

		template <DXGI_FORMAT Format>
		Texture2DUnorderedAccessView<Format> Texture2D::CreateUnorderedAccessView(const std::uint32_t mipSlice) const
		{
			assert(D3D12GetFormatPlaneCount(&(Util::Engine::GetD3D12Device()), GetResourceDescription().Format) == 1 && "Congratulations! You found a multi-planar texture format which isn't meant for depth/stencil textures. Have fun figuring out how to create descriptors for it~");

			return Texture2DUnorderedAccessView<Format>{ *this, D3D12_TEX2D_UAV{
				.MipSlice = mipSlice,
				.PlaneSlice = 0
			} };
		}

		template <DXGI_FORMAT Format>
		BindlessSRVAllocation Texture2D::CreateBindlessSRV()
		{
			return I_GPUResource::CreateBindlessSRV(CreateShaderResourceView<Format>().CreateSRVDescription());
		}
	}
}