module;
#include <cassert>
#include "DxDef.h"

export module Brawler.D3D12.DepthStencilView;
import :DSVDimensionInfo;
import Brawler.D3D12.I_GPUResource;

export namespace Brawler
{
	namespace D3D12
	{
		enum class DepthStencilAccessMode
		{
			DEPTH_READ_STENCIL_READ,
			DEPTH_READ_STENCIL_WRITE,
			DEPTH_WRITE_STENCIL_READ,
			DEPTH_WRITE_SENCIL_WRITE
		};
	}
}

namespace Brawler
{
	namespace D3D12
	{
		template <DXGI_FORMAT Format>
		consteval bool IsValidDXGIFormat()
		{
			// According to the MSDN at https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_depth_stencil_view_desc#remarks,
			// only the following formats are valid for depth/stencil views (DSVs):
			//
			//   - DXGI_FORMAT_D16_UNORM
			//   - DXGI_FORMAT_D24_UNORM_S8_UINT
			//   - DXGI_FORMAT_D32_FLOAT
			//   - DXGI_FORMAT_D32_FLOAT_S8X24_UINT
			//
			// DXGI_FORMAT_UNKNOWN is also valid, but it is interpreted by the D3D12 API as the DXGI_FORMAT
			// of the underlying ID3D12Resource object. We like to be explicit about our view types, so
			// we disallow that one.

			switch (Format)
			{
			case DXGI_FORMAT::DXGI_FORMAT_D16_UNORM: [[fallthrough]];
			case DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT: [[fallthrough]];
			case DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT: [[fallthrough]];
			case DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
				return true;

			default:
				return false;
			}
		}

		template <D3D12_DSV_DIMENSION ViewDimension>
		consteval bool IsValidDSVDimension()
		{
			// According to the MSDN, D3D12_DSV_DIMENSION_UNKNOWN is an invalid value and should
			// never be used to create a DSV. No, I don't know why they included it anyways.
			return (ViewDimension != D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_UNKNOWN);
		}

		template <DXGI_FORMAT Format, D3D12_DSV_DIMENSION ViewDimension>
		static constexpr bool IS_VALID_DEPTH_STENCIL_VIEW = (IsValidDXGIFormat<Format>() && IsValidDSVDimension<ViewDimension>());

		consteval D3D12_DSV_FLAGS GetDSVFlags(const DepthStencilAccessMode accessMode)
		{
			switch (accessMode)
			{
			case DepthStencilAccessMode::DEPTH_READ_STENCIL_READ:
				return (D3D12_DSV_FLAGS::D3D12_DSV_FLAG_READ_ONLY_DEPTH | D3D12_DSV_FLAGS::D3D12_DSV_FLAG_READ_ONLY_STENCIL);

			case DepthStencilAccessMode::DEPTH_READ_STENCIL_WRITE:
				return D3D12_DSV_FLAGS::D3D12_DSV_FLAG_READ_ONLY_DEPTH;

			case DepthStencilAccessMode::DEPTH_WRITE_STENCIL_READ:
				return D3D12_DSV_FLAGS::D3D12_DSV_FLAG_READ_ONLY_STENCIL;

			case DepthStencilAccessMode::DEPTH_WRITE_SENCIL_WRITE:
				return D3D12_DSV_FLAGS::D3D12_DSV_FLAG_NONE;

			default: [[unlikely]]
			{
				assert(false);
				return D3D12_DSV_FLAGS::D3D12_DSV_FLAG_NONE;
			}
			}
		}
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		template <DXGI_FORMAT Format, D3D12_DSV_DIMENSION ViewDimension, DepthStencilAccessMode AccessMode>
			requires IS_VALID_DEPTH_STENCIL_VIEW<Format, ViewDimension>
		class DepthStencilView
		{
		private:
			using ViewDescType = typename DSVDimensionInfo<ViewDimension>::ViewDescType;

			// NOTE: Unlike the other view types, Brawler::D3D12::ReinterpretResourceCast()
			// does not get an overload for DepthStencilView. This is because DSVs can never
			// be a typeless format, and so reinterpreting the resource through views is not
			// allowed.

		public:
			DepthStencilView() = default;
			DepthStencilView(const I_GPUResource& resource, ViewDescType&& viewDesc);

			DepthStencilView(const DepthStencilView& rhs) = default;
			DepthStencilView& operator=(const DepthStencilView& rhs) = default;

			DepthStencilView(DepthStencilView&& rhs) noexcept = default;
			DepthStencilView& operator=(DepthStencilView&& rhs) noexcept = default;

			const I_GPUResource& GetGPUResource() const;
			Brawler::D3D12Resource& GetD3D12Resource() const;

			D3D12_DEPTH_STENCIL_VIEW_DESC GetDSVDescription() const;

		private:
			const I_GPUResource* mResourcePtr;
			ViewDescType mViewDesc;
		};
	}
}

// ------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <DXGI_FORMAT Format, D3D12_DSV_DIMENSION ViewDimension, DepthStencilAccessMode AccessMode>
			requires IS_VALID_DEPTH_STENCIL_VIEW<Format, ViewDimension>
		DepthStencilView<Format, ViewDimension, AccessMode>::DepthStencilView(const I_GPUResource& resource, ViewDescType&& viewDesc) :
			mResourcePtr(&resource),
			mViewDesc(std::move(viewDesc))
		{}

		template <DXGI_FORMAT Format, D3D12_DSV_DIMENSION ViewDimension, DepthStencilAccessMode AccessMode>
			requires IS_VALID_DEPTH_STENCIL_VIEW<Format, ViewDimension>
		const I_GPUResource& DepthStencilView<Format, ViewDimension, AccessMode>::GetGPUResource() const
		{
			assert(mResourcePtr != nullptr && "ERROR: A DepthStencilView instance was never assigned an I_GPUResource instance!");
			return *mResourcePtr;
		}

		template <DXGI_FORMAT Format, D3D12_DSV_DIMENSION ViewDimension, DepthStencilAccessMode AccessMode>
			requires IS_VALID_DEPTH_STENCIL_VIEW<Format, ViewDimension>
		Brawler::D3D12Resource& DepthStencilView<Format, ViewDimension, AccessMode>::GetD3D12Resource() const
		{
			return GetGPUResource().GetD3D12Resource();
		}

		template <DXGI_FORMAT Format, D3D12_DSV_DIMENSION ViewDimension, DepthStencilAccessMode AccessMode>
			requires IS_VALID_DEPTH_STENCIL_VIEW<Format, ViewDimension>
		D3D12_DEPTH_STENCIL_VIEW_DESC DepthStencilView<Format, ViewDimension, AccessMode>::GetDSVDescription() const
		{
			static constexpr D3D12_DEPTH_STENCIL_VIEW_DESC DEFAULT_DSV_DESC{
				.Format = Format,
				.ViewDimension = ViewDimension,
				.Flags = GetDSVFlags(AccessMode)
			};

			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{ DEFAULT_DSV_DESC };
			InitializeDSVDescription<ViewDimension>(dsvDesc, mViewDesc);

			return dsvDesc;
		}
	}
}

// ------------------------------------------------------------------------------------------------------------------------------------------------

export namespace Brawler
{
	namespace D3D12
	{
		template <DXGI_FORMAT Format, DepthStencilAccessMode AccessMode>
		using Texture1DDepthStencilView = DepthStencilView<Format, D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE1D, AccessMode>;

		template <DXGI_FORMAT Format, DepthStencilAccessMode AccessMode>
		using Texture1DArrayDepthStencilView = DepthStencilView<Format, D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE1DARRAY, AccessMode>;

		template <DXGI_FORMAT Format, DepthStencilAccessMode AccessMode>
		using Texture2DDepthStencilView = DepthStencilView<Format, D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE2D, AccessMode>;

		template <DXGI_FORMAT Format, DepthStencilAccessMode AccessMode>
		using Texture2DArrayDepthStencilView = DepthStencilView<Format, D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE2DARRAY, AccessMode>;
	}
}