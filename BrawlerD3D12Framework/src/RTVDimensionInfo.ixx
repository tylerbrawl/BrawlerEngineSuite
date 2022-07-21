module;
#include "DxDef.h"

export module Brawler.D3D12.RenderTargetView:RTVDimensionInfo;

export namespace Brawler
{
	namespace D3D12
	{
		template <D3D12_RTV_DIMENSION ViewDimension>
		struct RTVDimensionInfo
		{
			static_assert(sizeof(ViewDimension) != sizeof(ViewDimension));
		};

		template <>
		struct RTVDimensionInfo<D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_BUFFER>
		{
			using ViewDescType = D3D12_BUFFER_RTV;
		};

		template <>
		struct RTVDimensionInfo<D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE1D>
		{
			using ViewDescType = D3D12_TEX1D_RTV;
		};

		template <>
		struct RTVDimensionInfo<D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE1DARRAY>
		{
			using ViewDescType = D3D12_TEX1D_ARRAY_RTV;
		};

		template <>
		struct RTVDimensionInfo<D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D>
		{
			using ViewDescType = D3D12_TEX2D_RTV;
		};

		template <>
		struct RTVDimensionInfo<D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2DARRAY>
		{
			using ViewDescType = D3D12_TEX2D_ARRAY_RTV;
		};

		template <>
		struct RTVDimensionInfo<D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2DMS>
		{
			using ViewDescType = D3D12_TEX2DMS_RTV;
		};

		template <>
		struct RTVDimensionInfo<D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY>
		{
			using ViewDescType = D3D12_TEX2DMS_ARRAY_RTV;
		};

		template <>
		struct RTVDimensionInfo<D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE3D>
		{
			using ViewDescType = D3D12_TEX3D_RTV;
		};
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		template <DXGI_FORMAT Format, D3D12_RTV_DIMENSION ViewDimension>
		void ModifyRTVDescription(D3D12_RENDER_TARGET_VIEW_DESC& rtvDesc, const typename RTVDimensionInfo<ViewDimension>::ViewDescType& viewDesc)
		{
			// Yes, this is ugly as heck. See SRVDimensionInfo.ixx for why we do it like this.

			if constexpr (ViewDimension == D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_BUFFER)
				rtvDesc.Buffer = viewDesc;
			else if constexpr (ViewDimension == D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE1D)
				rtvDesc.Texture1D = viewDesc;
			else if constexpr (ViewDimension == D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE1DARRAY)
				rtvDesc.Texture1DArray = viewDesc;
			else if constexpr (ViewDimension == D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D)
				rtvDesc.Texture2D = viewDesc;
			else if constexpr (ViewDimension == D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2DARRAY)
				rtvDesc.Texture2DArray = viewDesc;
			else if constexpr (ViewDimension == D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2DMS)
				rtvDesc.Texture2DMS = viewDesc;
			else if constexpr (ViewDimension == D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY)
				rtvDesc.Texture2DMSArray = viewDesc;
			else if constexpr (ViewDimension == D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE3D)
				rtvDesc.Texture3D = viewDesc;
		}
	}
}