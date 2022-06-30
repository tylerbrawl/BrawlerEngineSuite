// WARNING: This file was auto-generated by the Brawler Shader Compiler. You will incur the
// wrath of God if you dare touch it.

module;

export module Brawler.RootParameters.RootParameterEnums;
export import Brawler.D3D12.RootParameterType;

export namespace Brawler
{
	namespace RootParameters
	{
		enum class BC6HBC7Compression
		{
			SOURCE_TEXTURE_SRV_TABLE,
			INPUT_BUFFER_SRV_TABLE,
			OUTPUT_BUFFER_UAV_TABLE,
			COMPRESSION_SETTINGS_CBV,
			MODE_ID_AND_START_BLOCK_NUM_ROOT_CONSTANTS,

			COUNT_OR_ERROR
		};

		enum class GenericDownsample
		{
			TEXTURES_TABLE,
			MIP_MAP_CONSTANTS,

			COUNT_OR_ERROR
		};

		enum class VirtualTexturePageTiling
		{
			TILING_CONSTANTS,
			OUTPUT_TEXTURE_TABLE,
			INPUT_TEXTURE_TABLE,

			COUNT_OR_ERROR
		};
	}
}