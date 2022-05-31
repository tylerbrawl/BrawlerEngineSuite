module;
#include <cstdint>
#include <cassert>
#include "DxDef.h"

export module Brawler.D3D12.Texture2D;
import Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.Texture2DBuilders;
import Brawler.D3D12.GPUResourceViews;
import Util.Engine;
import Brawler.D3D12.TextureSubResource;

export namespace Brawler
{
	namespace D3D12
	{
		class Texture2D final : public I_GPUResource
		{
		public:
			explicit Texture2D(const Texture2DBuilder& builder);

			Texture2D(const Texture2D& rhs) = delete;
			Texture2D& operator=(const Texture2D& rhs) = delete;

			Texture2D(Texture2D&& rhs) noexcept = default;
			Texture2D& operator=(Texture2D&& rhs) noexcept = default;

			template <DXGI_FORMAT Format>
			Texture2DShaderResourceView<Format> CreateShaderResourceView();

			template <DXGI_FORMAT Format>
			Texture2DUnorderedAccessView<Format> CreateUnorderedAccessView(const std::uint32_t mipSlice = 0) const;

			/// <summary>
			/// Creates and returns a TextureSubResource instance which refers to the mip level
			/// of this Texture2D indicated by mipSlice. This directly corresponds to the idea of
			/// sub-resources from the D3D12 API.
			/// </summary>
			/// <param name="mipSlice">
			/// - The mip level for which a TextureSubResource instance will be created. If no
			///   value is specified, then a TextureSubResource instance is created for the highest
			///   quality mip level (i.e., mipSlice == 0).
			/// </param>
			/// <returns>
			/// The function returns a TextureSubResource instance which refers to the mip level
			/// of this Texture2D indicated by mipSlice.
			/// </returns>
			TextureSubResource GetSubResource(const std::uint32_t mipSlice = 0);
		};
	}
}

// --------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <DXGI_FORMAT Format>
		Texture2DShaderResourceView<Format> Texture2D::CreateShaderResourceView()
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
	}
}