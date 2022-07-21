module;
#include "DxDef.h"

export module Brawler.D3D12.RenderTargetView;
import :RTVDimensionInfo;
import Brawler.D3D12.I_GPUResource;
import Util.D3D12;

namespace Brawler
{
	namespace D3D12
	{
		template <DXGI_FORMAT Format, D3D12_RTV_DIMENSION ViewDimension>
		class RenderTargetView;

		template <DXGI_FORMAT ToFormat, DXGI_FORMAT FromFormat, D3D12_RTV_DIMENSION ViewDimension>
		RenderTargetView<ToFormat, ViewDimension> ReinterpretResourceCast(const RenderTargetView<FromFormat, ViewDimension>& srcRTV);

		consteval bool IsFormatTypeless(const DXGI_FORMAT format)
		{
			switch (format)
			{
			case DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_TYPELESS: [[fallthrough]];
			case DXGI_FORMAT::DXGI_FORMAT_R32G32B32_TYPELESS: [[fallthrough]];
			case DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_TYPELESS: [[fallthrough]];
			case DXGI_FORMAT::DXGI_FORMAT_R32G32_TYPELESS: [[fallthrough]];
			case DXGI_FORMAT::DXGI_FORMAT_R32G8X24_TYPELESS: [[fallthrough]];
			case DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS: [[fallthrough]];
			case DXGI_FORMAT::DXGI_FORMAT_X32_TYPELESS_G8X24_UINT: [[fallthrough]];
			case DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_TYPELESS: [[fallthrough]];
			case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_TYPELESS: [[fallthrough]];
			case DXGI_FORMAT::DXGI_FORMAT_R16G16_TYPELESS: [[fallthrough]];
			case DXGI_FORMAT::DXGI_FORMAT_R32_TYPELESS: [[fallthrough]];
			case DXGI_FORMAT::DXGI_FORMAT_R24G8_TYPELESS: [[fallthrough]];
			case DXGI_FORMAT::DXGI_FORMAT_R24_UNORM_X8_TYPELESS: [[fallthrough]];
			case DXGI_FORMAT::DXGI_FORMAT_X24_TYPELESS_G8_UINT: [[fallthrough]];
			case DXGI_FORMAT::DXGI_FORMAT_R8G8_TYPELESS: [[fallthrough]];
			case DXGI_FORMAT::DXGI_FORMAT_R16_TYPELESS: [[fallthrough]];
			case DXGI_FORMAT::DXGI_FORMAT_R8_TYPELESS: [[fallthrough]];
			case DXGI_FORMAT::DXGI_FORMAT_BC1_TYPELESS: [[fallthrough]];
			case DXGI_FORMAT::DXGI_FORMAT_BC2_TYPELESS: [[fallthrough]];
			case DXGI_FORMAT::DXGI_FORMAT_BC3_TYPELESS: [[fallthrough]];
			case DXGI_FORMAT::DXGI_FORMAT_BC4_TYPELESS: [[fallthrough]];
			case DXGI_FORMAT::DXGI_FORMAT_BC5_TYPELESS: [[fallthrough]];
			case DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_TYPELESS: [[fallthrough]];
			case DXGI_FORMAT::DXGI_FORMAT_B8G8R8X8_TYPELESS: [[fallthrough]];
			case DXGI_FORMAT::DXGI_FORMAT_BC6H_TYPELESS: [[fallthrough]];
			case DXGI_FORMAT::DXGI_FORMAT_BC7_TYPELESS:
				return true;

			default:
				return false;
			}
		}

		consteval bool IsR32G32B32Format(const DXGI_FORMAT format)
		{
			switch (format)
			{
			case DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT: [[fallthrough]];
			case DXGI_FORMAT::DXGI_FORMAT_R32G32B32_SINT: [[fallthrough]];
			case DXGI_FORMAT::DXGI_FORMAT_R32G32B32_TYPELESS: [[fallthrough]];
			case DXGI_FORMAT::DXGI_FORMAT_R32G32B32_UINT:
				return true;

			default:
				return false;
			}
		}
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		template <DXGI_FORMAT Format, D3D12_RTV_DIMENSION ViewDimension>
		class RenderTargetView
		{
		private:
			template <DXGI_FORMAT ToFormat, DXGI_FORMAT FromFormat>
			friend RenderTargetView<ToFormat, ViewDimension> ReinterpretResourceCast(const RenderTargetView<FromFormat, ViewDimension>& srcRTV);

		private:
			using ViewDescType = typename RTVDimensionInfo<ViewDimension>::ViewDescType;

		public:
			RenderTargetView() = default;
			RenderTargetView(const I_GPUResource& resource, ViewDescType&& viewDesc);

			RenderTargetView(const RenderTargetView& rhs) = default;
			RenderTargetView& operator=(const RenderTargetView& rhs) = default;

			RenderTargetView(RenderTargetView&& rhs) noexcept = default;
			RenderTargetView& operator=(RenderTargetView&& rhs) noexcept = default;

			const I_GPUResource& GetGPUResource() const;
			Brawler::D3D12Resource& GetD3D12Resource() const;

			D3D12_RENDER_TARGET_VIEW_DESC CreateRTVDescription() const;

		private:
			const I_GPUResource* mResourcePtr;
			ViewDescType mViewDesc;
		};
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <DXGI_FORMAT Format, D3D12_RTV_DIMENSION ViewDimension>
		RenderTargetView<Format, ViewDimension>::RenderTargetView(const I_GPUResource& resource, ViewDescType&& viewDesc) :
			mResourcePtr(&resource),
			mViewDesc(std::move(viewDesc))
		{
			assert((mResourcePtr->GetResourceDescription().Flags & D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0 && "ERROR: An attempt was made to create a RenderTargetView for an I_GPUResource whose D3D12_RESOURCE_DESC did not have the D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET flag set!");
		}

		template <DXGI_FORMAT Format, D3D12_RTV_DIMENSION ViewDimension>
		const I_GPUResource& RenderTargetView<Format, ViewDimension>::GetGPUResource() const
		{
			assert(mResourcePtr != nullptr);
			return *mResourcePtr;
		}

		template <DXGI_FORMAT Format, D3D12_RTV_DIMENSION ViewDimension>
		Brawler::D3D12Resource& RenderTargetView<Format, ViewDimension>::GetD3D12Resource() const
		{
			return GetGPUResource().GetD3D12Resource();
		}

		template <DXGI_FORMAT Format, D3D12_RTV_DIMENSION ViewDimension>
		D3D12_RENDER_TARGET_VIEW_DESC RenderTargetView<Format, ViewDimension>::CreateRTVDescription() const
		{
			static constexpr D3D12_RENDER_TARGET_VIEW_DESC DEFAULT_RTV_DESC{
				.Format = Format,
				.ViewDimension = ViewDimension
			};

			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{ DEFAULT_RTV_DESC };
			ModifyRTVDescription<Format, ViewDimension>(rtvDesc, mViewDesc);

			return rtvDesc;
		}
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------------------------

// NOTE: RTVs cannot be made from typeless formats. In addition, the format cannot be DXGI_FORMAT_R32G32B32_X
// for buffers.

export namespace Brawler
{
	namespace D3D12
	{
		template <DXGI_FORMAT Format>
			requires (!IsFormatTypeless(Format) && !IsR32G32B32Format(Format))
		using TypedBufferRenderTargetView = RenderTargetView<Format, D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_BUFFER>;

		template <DXGI_FORMAT Format>
			requires !IsFormatTypeless(Format)
		using Texture1DRenderTargetView = RenderTargetView<Format, D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE1D>;

		template <DXGI_FORMAT Format>
			requires !IsFormatTypeless(Format)
		using Texture1DArrayRenderTargetView = RenderTargetView<Format, D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE1DARRAY>;

		template <DXGI_FORMAT Format>
			requires !IsFormatTypeless(Format)
		using Texture2DRenderTargetView = RenderTargetView<Format, D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D>;

		template <DXGI_FORMAT Format>
			requires !IsFormatTypeless(Format)
		using Texture2DArrayRenderTargetView = RenderTargetView<Format, D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2DARRAY>;

		// TODO: Do we really want to expose the multisampled 2D texture RTV types?

		template <DXGI_FORMAT Format>
			requires !IsFormatTypeless(Format)
		using Texture3DRenderTargetView = RenderTargetView<Format, D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE3D>;
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------------------------

export namespace Brawler
{
	namespace D3D12
	{
		template <DXGI_FORMAT ToFormat, DXGI_FORMAT FromFormat, D3D12_RTV_DIMENSION ViewDimension>
		RenderTargetView<ToFormat, ViewDimension> ReinterpretResourceCast(const RenderTargetView<FromFormat, ViewDimension>& srcRTV)
		{
			assert(Util::D3D12::IsSRVRTVDSVResourceCastLegal(FromFormat, ToFormat) && "ERROR: An attempt was made to cast a resource to a different DXGI_FORMAT, but this cast is considered illegal! (Don't bother looking online for the casting rules. Just check the comments in D3D12UtilFormats.ixx.)");

			RenderTargetView<ToFormat, ViewDimension> destRTV{};
			destRTV.mResourcePtr = srcRTV.mResourcePtr;
			destRTV.mViewDesc = srcRTV.mViewDesc;

			return destRTV;
		}
	}
}