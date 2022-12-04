module;
#include <cassert>
#include <optional>
#include "DxDef.h"

export module Brawler.D3D12.DepthStencilTexture;
import Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.TextureSubResource;
import Brawler.D3D12.Texture2DBuilders;
import Brawler.D3D12.GPUResourceSpecialInitializationMethod;
import Brawler.D3D12.GPUResourceViews;
import Util.D3D12;
import Brawler.D3D12.BindlessSRVAllocation;

namespace Brawler
{
	namespace D3D12
	{
		constexpr bool DoesFormatHaveStencilPlane(const DXGI_FORMAT textureFormat)
		{
			return (textureFormat == DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT || textureFormat == DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT);
		}
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class DepthStencilTexture;
	}
}

namespace Brawler
{
	namespace D3D12
	{
		enum class DepthStencilPlaneID
		{
			DEPTH,
			STENCIL
		};

		template <DepthStencilPlaneID PlaneID>
		class DepthStencilTextureSubResource final : public TextureSubResource
		{
		public:
			DepthStencilTextureSubResource() = default;
			DepthStencilTextureSubResource(DepthStencilTexture& depthStencilTexture, const std::uint32_t subResourceIndex);

			DepthStencilTextureSubResource(const DepthStencilTextureSubResource& rhs) = default;
			DepthStencilTextureSubResource& operator=(const DepthStencilTextureSubResource& rhs) = default;

			DepthStencilTextureSubResource(DepthStencilTextureSubResource&& rhs) noexcept = default;
			DepthStencilTextureSubResource& operator=(DepthStencilTextureSubResource&& rhs) noexcept = default;

			I_GPUResource& GetGPUResource() override;
			const I_GPUResource& GetGPUResource() const override;

			DepthStencilTexture& GetDepthStencilTexture();
			const DepthStencilTexture& GetDepthStencilTexture() const;

			template <DXGI_FORMAT Format, DepthStencilAccessMode AccessMode>
			Texture2DDepthStencilView<Format, AccessMode> CreateDepthStencilView() const;

			template <DXGI_FORMAT Format>
			Texture2DShaderResourceView<Format> CreateShaderResourceView() const;

			template <DXGI_FORMAT Format>
			BindlessSRVAllocation CreateBindlessSRV();

		private:
			bool CanCreateShaderResourceViews() const;

			std::uint32_t GetMipSlice() const;

		private:
			DepthStencilTexture* mTexturePtr;
		};
	}
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <DepthStencilPlaneID PlaneID>
		template <DXGI_FORMAT Format, DepthStencilAccessMode AccessMode>
		Texture2DDepthStencilView<Format, AccessMode> DepthStencilTextureSubResource<PlaneID>::CreateDepthStencilView() const
		{
			assert(Util::D3D12::IsSRVRTVDSVResourceCastLegal(GetDepthStencilTexture().GetResourceDescription().Format, Format) && "ERROR: An attempt was made to create a DepthStencilView using a DepthStencilTextureSubResource with a DXGI_FORMAT which was incompatible with that of the underlying ID3D12Resource of the relevant DepthStencilTexture!");

			return Texture2DDepthStencilView<Format, AccessMode>{ GetDepthStencilTexture(), D3D12_TEX2D_DSV{
				.MipSlice = GetMipSlice()
			} };
		}

		template <DepthStencilPlaneID PlaneID>
		template <DXGI_FORMAT Format>
		Texture2DShaderResourceView<Format> DepthStencilTextureSubResource<PlaneID>::CreateShaderResourceView() const
		{
			assert(Util::D3D12::IsSRVRTVDSVResourceCastLegal(GetDepthStencilTexture().GetResourceDescription().Format, Format) && "ERROR: An attempt was made to create a ShaderResourceView from a DepthStencilTextureSubResource with a DXGI_FORMAT which was incompatible with that of the underlying ID3D12Resource!");
			assert(CanCreateShaderResourceViews() && "ERROR: An attempt was made to create a ShaderResourceView using a DepthStencilTextureSubResource instance, but SRVs were marked as denied in the resource description of the relevant DepthStencilTexture!");

			static constexpr std::uint32_t PLANE_SLICE = std::to_underlying(PlaneID);

			return Texture2DShaderResourceView<Format>{ GetDepthStencilTexture(), D3D12_TEX2D_SRV{
				.MostDetailedMip = GetMipSlice(),
				.MipLevels = 1,
				.PlaneSlice = PLANE_SLICE,
				.ResourceMinLODClamp = 0.0f
			} };
		}

		template <DepthStencilPlaneID PlaneID>
		template <DXGI_FORMAT Format>
		BindlessSRVAllocation DepthStencilTextureSubResource<PlaneID>::CreateBindlessSRV()
		{
			return (static_cast<I_GPUResource&>(GetDepthStencilTexture()).CreateBindlessSRV(CreateShaderResourceView<Format>().CreateSRVDescription()));
		}
	}
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

export namespace Brawler
{
	namespace D3D12
	{
		using DepthTextureSubResource = DepthStencilTextureSubResource<DepthStencilPlaneID::DEPTH>;
		using StencilTextureSubResource = DepthStencilTextureSubResource<DepthStencilPlaneID::STENCIL>;
	}
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

export namespace Brawler
{
	namespace D3D12
	{
		class DepthStencilTexture final : public I_GPUResource
		{
		public:
			explicit DepthStencilTexture(const DepthStencilTextureBuilder& builder);

			DepthStencilTexture(const DepthStencilTexture& rhs) = delete;
			DepthStencilTexture& operator=(const DepthStencilTexture& rhs) = delete;

			DepthStencilTexture(DepthStencilTexture&& rhs) noexcept = default;
			DepthStencilTexture& operator=(DepthStencilTexture&& rhs) noexcept = default;

			template <DXGI_FORMAT Format, DepthStencilAccessMode AccessMode>
			Texture2DDepthStencilView<Format, AccessMode> CreateDepthStencilView(const std::uint32_t mipSlice = 0) const;

			template <DXGI_FORMAT Format>
			Texture2DShaderResourceView<Format> CreateShaderResourceViewForDepthPlane(const std::uint32_t mostDetailedMipOffset = 0) const;

			template <DXGI_FORMAT Format>
			Texture2DShaderResourceView<Format> CreateShaderResourceViewForStencilPlane(const std::uint32_t mostDetailedMipOffset = 0) const;

			std::optional<D3D12_CLEAR_VALUE> GetOptimizedClearValue() const override;
			GPUResourceSpecialInitializationMethod GetPreferredSpecialInitializationMethod() const override;

			DepthTextureSubResource GetDepthTextureSubResource(const std::uint32_t mipSlice = 0);
			StencilTextureSubResource GetStencilTextureSubResource(const std::uint32_t mipSlice = 0);

			template <DXGI_FORMAT Format>
			BindlessSRVAllocation CreateBindlessSRVForDepthPlane(const std::uint32_t mostDetailedMipOffset = 0);

			template <DXGI_FORMAT Format>
			BindlessSRVAllocation CreateBindlessSRVForStencilPlane(const std::uint32_t mostDetailedMipOffset = 0);

		private:
			bool CanCreateShaderResourceViews() const;

		private:
			std::optional<D3D12_CLEAR_VALUE> mOptimizedClearValue;
			GPUResourceSpecialInitializationMethod mInitMethod;
		};
	}
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <DXGI_FORMAT Format, DepthStencilAccessMode AccessMode>
		Texture2DDepthStencilView<Format, AccessMode> DepthStencilTexture::CreateDepthStencilView(const std::uint32_t mipSlice) const
		{
			assert(Util::D3D12::IsSRVRTVDSVResourceCastLegal(GetResourceDescription().Format, Format) && "ERROR: An attempt was made to create a DepthStencilView for a DepthStencilTexture with a DXGI_FORMAT which was incompatible with that of the underlying ID3D12Resource!");

			return Texture2DDepthStencilView<Format, AccessMode>{ *this, D3D12_TEX2D_DSV{
				.MipSlice = mipSlice
			} };
		}

		template <DXGI_FORMAT Format>
		Texture2DShaderResourceView<Format> DepthStencilTexture::CreateShaderResourceViewForDepthPlane(const std::uint32_t mostDetailedMipOffset) const
		{
			assert(Util::D3D12::IsSRVRTVDSVResourceCastLegal(GetResourceDescription().Format, Format) && "ERROR: An attempt was made to create a ShaderResourceView for a DepthStencilTexture's depth plane with a DXGI_FORMAT which was incompatible with that of the underlying ID3D12Resource!");
			assert(CanCreateShaderResourceViews() && "ERROR: An attempt was made to create a ShaderResourceView for a DepthStencilTexture instance, but SRVs were marked as denied in the resource description of said instance!");

			static constexpr std::uint32_t DEPTH_PLANE_SLICE = std::to_underlying(DepthStencilPlaneID::DEPTH);

			return Texture2DShaderResourceView<Format>{ *this, D3D12_TEX2D_SRV{
				.MostDetailedMip = mostDetailedMipOffset,
				.MipLevels = static_cast<std::uint32_t>(-1),
				.PlaneSlice = DEPTH_PLANE_SLICE,
				.ResourceMinLODClamp = 0.0f
			} };
		}

		template <DXGI_FORMAT Format>
		Texture2DShaderResourceView<Format> DepthStencilTexture::CreateShaderResourceViewForStencilPlane(const std::uint32_t mostDetailedMipOffset) const
		{
			assert(Util::D3D12::IsSRVRTVDSVResourceCastLegal(GetResourceDescription().Format, Format) && "ERROR: An attempt was made to create a ShaderResourceView for a DepthStencilTexture's stencil plane with a DXGI_FORMAT which was incompatible with that of the underlying ID3D12Resource!");
			assert(CanCreateShaderResourceViews() && "ERROR: An attempt was made to create a ShaderResourceView for a DepthStencilTexture instance, but SRVs were marked as denied in the resource description of said instance!");
			assert(DoesFormatHaveStencilPlane(GetResourceDescription().Format) && "ERROR: An attempt was made to create a ShaderResourceView for the stencil plane of a DepthStencilTexture instance which doesn't have one!");

			static constexpr std::uint32_t STENCIL_PLANE_SLICE = std::to_underlying(DepthStencilPlaneID::STENCIL);

			return Texture2DShaderResourceView<Format>{ *this, D3D12_TEX2D_SRV{
				.MostDetailedMip = mostDetailedMipOffset,
				.MipLevels = static_cast<std::uint32_t>(-1),
				.PlaneSlice = STENCIL_PLANE_SLICE,
				.ResourceMinLODClamp = 0.0f
			} };
		}

		template <DXGI_FORMAT Format>
		BindlessSRVAllocation DepthStencilTexture::CreateBindlessSRVForDepthPlane(const std::uint32_t mostDetailedMipOffset)
		{
			return (static_cast<I_GPUResource&>(*this).CreateBindlessSRV(CreateShaderResourceViewForDepthPlane<Format>(mostDetailedMipOffset).CreateSRVDescription()));
		}

		template <DXGI_FORMAT Format>
		BindlessSRVAllocation DepthStencilTexture::CreateBindlessSRVForStencilPlane(const std::uint32_t mostDetailedMipOffset)
		{
			return (static_cast<I_GPUResource&>(*this).CreateBindlessSRV(CreateShaderResourceViewForStencilPlane<Format>(mostDetailedMipOffset).CreateSRVDescription()));
		}
	}
}