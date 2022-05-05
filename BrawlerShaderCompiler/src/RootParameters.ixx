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