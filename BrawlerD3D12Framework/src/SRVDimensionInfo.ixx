module;
#include "DxDef.h"

export module Brawler.D3D12.ShaderResourceView:SRVDimensionInfo;

namespace Brawler
{
	namespace D3D12
	{
		template <std::underlying_type_t<D3D12_SRV_DIMENSION> ViewDimension>
		struct SRVDimensionInfo
		{
			static_assert(sizeof(ViewDimension) != sizeof(ViewDimension));
		};

		template <>
		struct SRVDimensionInfo<std::to_underlying(D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_BUFFER)>
		{
			using SRVDescType = D3D12_BUFFER_SRV;
		};

		template <>
		struct SRVDimensionInfo<std::to_underlying(D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE1D)>
		{
			using SRVDescType = D3D12_TEX1D_SRV;
		};

		template <>
		struct SRVDimensionInfo<std::to_underlying(D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE1DARRAY)>
		{
			using SRVDescType = D3D12_TEX1D_ARRAY_SRV;
		};

		template <>
		struct SRVDimensionInfo<std::to_underlying(D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D)>
		{
			using SRVDescType = D3D12_TEX2D_SRV;
		};

		template <>
		struct SRVDimensionInfo<std::to_underlying(D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2DARRAY)>
		{
			using SRVDescType = D3D12_TEX2D_ARRAY_SRV;
		};

		template <>
		struct SRVDimensionInfo<std::to_underlying(D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2DMS)>
		{
			using SRVDescType = D3D12_TEX2DMS_SRV;
		};

		template <>
		struct SRVDimensionInfo<std::to_underlying(D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY)>
		{
			using SRVDescType = D3D12_TEX2DMS_ARRAY_SRV;
		};

		template <>
		struct SRVDimensionInfo<std::to_underlying(D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE3D)>
		{
			using SRVDescType = D3D12_TEX3D_SRV;
		};

		template <>
		struct SRVDimensionInfo<std::to_underlying(D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURECUBE)>
		{
			using SRVDescType = D3D12_TEXCUBE_SRV;
		};

		template <>
		struct SRVDimensionInfo<std::to_underlying(D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURECUBEARRAY)>
		{
			using SRVDescType = D3D12_TEXCUBE_ARRAY_SRV;
		};

		template <>
		struct SRVDimensionInfo<std::to_underlying(D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE)>
		{
			using SRVDescType = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_SRV;
		};
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		template <D3D12_SRV_DIMENSION ViewDimension>
		__forceinline static constexpr void InitializeSRVDescription(D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc, const typename SRVDimensionInfo<ViewDimension>::SRVDescType& viewDesc)
		{
			// Originally, I had a separate InitializeSRVDescription() function within each explicit template specialization
			// of SRVDimensionInfo which would initialize the correct field of srvDesc. However, as of writing this, a regression
			// in the MSVC causes it to crash with a stack overflow error during parsing using that method. Instead, we now have
			// this behemoth of a function.
			
			if constexpr (ViewDimension == D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_BUFFER)
				srvDesc.Buffer = viewDesc;
			else if constexpr (ViewDimension == D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE1D)
				srvDesc.Texture1D = viewDesc;
			else if constexpr (ViewDimension == D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE1DARRAY)
				srvDesc.Texture1DArray = viewDesc;
			else if constexpr (ViewDimension == D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D)
				srvDesc.Texture2D = viewDesc;
			else if constexpr (ViewDimension == D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2DARRAY)
				srvDesc.Texture2DArray = viewDesc;
			else if constexpr (ViewDimension == D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2DMS)
				srvDesc.Texture2DMS = viewDesc;
			else if constexpr (ViewDimension == D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY)
				srvDesc.Texture2DMSArray = viewDesc;
			else if constexpr (ViewDimension == D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE3D)
				srvDesc.Texture3D = viewDesc;
			else if constexpr (ViewDimension == D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURECUBE)
				srvDesc.TextureCube = viewDesc;
			else if constexpr (ViewDimension == D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURECUBEARRAY)
				srvDesc.TextureCubeArray = viewDesc;
			else if constexpr (ViewDimension == D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE)
				srvDesc.RaytracingAccelerationStructure = viewDesc;
		}
	}
}