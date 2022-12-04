module;
#include "DxDef.h"

export module Brawler.D3D12.DepthStencilView:DSVDimensionInfo;

export namespace Brawler
{
	namespace D3D12
	{
		template <D3D12_DSV_DIMENSION ViewDimension>
		struct DSVDimensionInfo
		{
			static_assert(sizeof(ViewDimension) != sizeof(ViewDimension));
		};

		template <typename ViewDescType_>
		struct DSVDimensionInfoInstantiation
		{
			using ViewDescType = ViewDescType_;
		};

		template <>
		struct DSVDimensionInfo<D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE1D> : public DSVDimensionInfoInstantiation<D3D12_TEX1D_DSV>
		{
			static constexpr void InitializeDSVDescription(D3D12_DEPTH_STENCIL_VIEW_DESC& dsvDesc, const ViewDescType& tex1DDesc)
			{
				dsvDesc.Texture1D = tex1DDesc;
			}
		};

		template <>
		struct DSVDimensionInfo<D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE1DARRAY> : public DSVDimensionInfoInstantiation<D3D12_TEX1D_ARRAY_DSV>
		{
			static constexpr void InitializeDSVDescription(D3D12_DEPTH_STENCIL_VIEW_DESC& dsvDesc, const ViewDescType& tex1DArrayDesc)
			{
				dsvDesc.Texture1DArray = tex1DArrayDesc;
			}
		};

		template <>
		struct DSVDimensionInfo<D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE2D> : public DSVDimensionInfoInstantiation<D3D12_TEX2D_DSV>
		{
			static constexpr void InitializeDSVDescription(D3D12_DEPTH_STENCIL_VIEW_DESC& dsvDesc, const ViewDescType& tex2DDesc)
			{
				dsvDesc.Texture2D = tex2DDesc;
			}
		};

		template <>
		struct DSVDimensionInfo<D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE2DARRAY> : public DSVDimensionInfoInstantiation<D3D12_TEX2D_ARRAY_DSV>
		{
			static constexpr void InitializeDSVDescription(D3D12_DEPTH_STENCIL_VIEW_DESC& dsvDesc, const ViewDescType& tex2DArrayDesc)
			{
				dsvDesc.Texture2DArray = tex2DArrayDesc;
			}
		};

		template <>
		struct DSVDimensionInfo<D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE2DMS> : public DSVDimensionInfoInstantiation<D3D12_TEX2DMS_DSV>
		{
			static constexpr void InitializeDSVDescription(D3D12_DEPTH_STENCIL_VIEW_DESC& dsvDesc, const ViewDescType& tex2DMultisampledDesc)
			{
				dsvDesc.Texture2DMS = tex2DMultisampledDesc;
			}
		};

		template <>
		struct DSVDimensionInfo<D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY> : public DSVDimensionInfoInstantiation<D3D12_TEX2DMS_ARRAY_DSV>
		{
			static constexpr void InitializeDSVDescription(D3D12_DEPTH_STENCIL_VIEW_DESC& dsvDesc, const ViewDescType& tex2DMultisampledArrayDesc)
			{
				dsvDesc.Texture2DMSArray = tex2DMultisampledArrayDesc;
			}
		};
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		template <D3D12_DSV_DIMENSION ViewDimension>
		constexpr void InitializeDSVDescription(D3D12_DEPTH_STENCIL_VIEW_DESC& dsvDesc, const typename DSVDimensionInfo<ViewDimension>::ViewDescType& dimensionViewDesc)
		{
			DSVDimensionInfo<ViewDimension>::InitializeDSVDescription(dsvDesc, dimensionViewDesc);
		}
	}
}