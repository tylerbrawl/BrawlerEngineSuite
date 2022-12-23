module;
#include "DxDef.h"

export module Brawler.RootParameters;

export namespace Brawler
{
	namespace RootParameters
	{
		enum class RootParameterType
		{
			CBV,
			SRV,
			UAV,
			ROOT_CONSTANT,
			DESCRIPTOR_TABLE
		};

		// Brawler Engine
		enum class DeferredGeometryRaster
		{
			/// <summary>
			/// Root Parameter 0: RootConstants<1> RasterConstants -> Space0[b0];
			/// </summary>
			RASTER_CONSTANTS,

			/// <summary>
			/// Root Parameter 1: DescriptorTable
			/// {
			///		SRV BindlessResources -> Space1-Space?[t0-t499999];
			/// };
			/// </summary>
			BINDLESS_SRVS,

			COUNT_OR_ERROR
		};

		// Brawler Model Exporter
		enum class BC6HBC7Compression
		{
			/// <summary>
			/// Root Parameter 0: DescriptorTable
			/// {
			///		SRV SourceTexture -> Space0[t0];
			/// };
			/// </summary>
			SOURCE_TEXTURE_SRV_TABLE,

			/// <summary>
			/// Root Parameter 1: DescriptorTable
			/// {
			///		SRV InputBuffer -> Space0[t1];
			/// };
			/// </summary>
			INPUT_BUFFER_SRV_TABLE,

			/// <summary>
			/// Root Parameter 2: DescriptorTable
			/// {
			///		UAV OutputBuffer -> Space0[u0];
			/// };
			/// </summary>
			OUTPUT_BUFFER_UAV_TABLE,

			/// <summary>
			/// Root Parameter 3: CBV CompressionSettings -> Space0[b0];
			/// </summary>
			COMPRESSION_SETTINGS_CBV,

			/// <summary>
			/// Root Parameter 4: RootConstants<2> ModeIDAndStartBlockNum -> Space0[b1];
			/// </summary>
			MODE_ID_AND_START_BLOCK_NUM_ROOT_CONSTANTS,

			COUNT_OR_ERROR
		};

		// Brawler Model Exporter
		enum class GenericDownsample
		{
			/// <summary>
			/// Root Parameter 0: DescriptorTable
			/// {
			///		STATIC_AT_EXECUTE SRV InputTexture -> Space0[t0];
			///		VOLATILE UAV OutputTexture1 -> Space0[u0];
			///		VOLATILE UAV OutputTexture2 -> Space0[u1];
			/// };
			/// </summary>
			TEXTURES_TABLE,

			/// <summary>
			/// Root Parameter 1: RootConstants<4> -> Space0[b0];
			/// </summary>
			MIP_MAP_CONSTANTS,

			COUNT_OR_ERROR
		};

		// Brawler Model Exporter
		enum class VirtualTexturePageTiling
		{
			/// <summary>
			/// Root Parameter 0: RootConstants<3> TilingConstants -> Space0[b0];
			/// </summary>
			TILING_ROOT_CONSTANTS,

			/// <summary>
			/// Root Parameter 1: Descriptor Table
			/// {
			///		VOLATILE UAV OutputPage0 -> Space0[u0];
			///		VOLATILE UAV OutputPage1 -> Space0[u1];
			///		VOLATILE UAV OutputPage2 -> Space0[u2];
			///		VOLATILE UAV OutputPage3 -> Space0[u3];
			/// };
			/// </summary>
			OUTPUT_PAGES_TABLE,

			/// <summary>
			/// Root Parameter 2: STATIC CBV MipLevelConstants -> Space0[b1];
			/// </summary>
			MIP_LEVEL_INFO_CBV,

			/// <summary>
			/// Root Parameter 3: Descriptor Table
			/// {
			///		STATIC SRV InputTexture -> Space0[t0];
			/// };
			/// </summary>
			INPUT_TEXTURE_TABLE,

			COUNT_OR_ERROR
		};

		// Brawler Model Exporter
		enum class VirtualTexturePageMerging
		{
			/// <summary>
			/// Root Parameter 0: RootConstants<2> TilingConstants -> Space0[b0];
			/// </summary>
			TILING_CONSTANTS,

			/// <summary>
			/// Root Parameter 1: DescriptorTable
			/// {
			///		VOLATILE UAV OutputTexture -> Space0[u0];
			/// };
			/// </summary>
			OUTPUT_TEXTURE_TABLE,

			/// <summary>
			/// Root Parameter 2: DescriptorTable
			/// {
			///		STATIC SRV InputTexture -> Space0[t0];
			/// };
			/// </summary>
			INPUT_TEXTURE_TABLE,

			COUNT_OR_ERROR
		};

		// Test Shader Profile
		enum class TestRootSignature
		{
			PARAM_0,
			PARAM_1,
			PARAM_2,
			PARAM_3,
			PARAM_4,
			PARAM_5,
			PARAM_6,

			COUNT_OR_ERROR
		};
	}
}