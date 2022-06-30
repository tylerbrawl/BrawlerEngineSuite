module;
#include "DxDef.h"

export module Brawler.D3D12.UnorderedAccessView:UAVDimensionInfo;

export namespace Brawler
{
	namespace D3D12
	{
		template <D3D12_UAV_DIMENSION ViewDimension>
		struct UAVDimensionInfo
		{
			static_assert(sizeof(ViewDimension) != sizeof(ViewDimension));
		};

		template <>
		struct UAVDimensionInfo<D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_BUFFER>
		{
			using UAVDescType = D3D12_BUFFER_UAV;
		};

		template <>
		struct UAVDimensionInfo<D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE1D>
		{
			using UAVDescType = D3D12_TEX1D_UAV;
		};

		template <>
		struct UAVDimensionInfo<D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE1DARRAY>
		{
			using UAVDescType = D3D12_TEX1D_ARRAY_UAV;
		};

		template <>
		struct UAVDimensionInfo<D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2D>
		{
			using UAVDescType = D3D12_TEX2D_UAV;
		};

		template <>
		struct UAVDimensionInfo<D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2DARRAY>
		{
			using UAVDescType = D3D12_TEX2D_ARRAY_UAV;
		};

		template <>
		struct UAVDimensionInfo<D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE3D>
		{
			using UAVDescType = D3D12_TEX3D_UAV;
		};
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		template <D3D12_UAV_DIMENSION ViewDimension>
		__forceinline static constexpr void InitializeUAVDescription(D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc, const typename UAVDimensionInfo<ViewDimension>::UAVDescType& viewDesc)
		{
			// Originally, I had a separate InitializeUAVDescription() function within each explicit template specialization
			// of UAVDimensionInfo which would initialize the correct field of uavDesc. However, as of writing this, a regression
			// in the MSVC causes it to crash with a stack overflow error during parsing using that method. Instead, we now have
			// this behemoth of a function.
			
			if constexpr (ViewDimension == D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_BUFFER)
				uavDesc.Buffer = viewDesc;
			else if constexpr (ViewDimension == D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE1D)
				uavDesc.Texture1D = viewDesc;
			else if constexpr (ViewDimension == D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE1DARRAY)
				uavDesc.Texture1DArray = viewDesc;
			else if constexpr (ViewDimension == D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2D)
				uavDesc.Texture2D = viewDesc;
			else if constexpr (ViewDimension == D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2DARRAY)
				uavDesc.Texture2DArray = viewDesc;
			else if constexpr (ViewDimension == D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE3D)
				uavDesc.Texture3D = viewDesc;
		}
	}
}