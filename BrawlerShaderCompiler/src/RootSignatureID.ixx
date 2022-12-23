module;

export module Brawler.RootSignatureID;

export namespace Brawler
{
	enum class RootSignatureID
	{
		// Brawler Engine
		DEFERRED_GEOMETRY_RASTER,
		MODEL_INSTANCE_FRUSTUM_CULL,
		DEFERRED_OPAQUE_SHADE,
		
		// Brawler Model Exporter
		BC6H_BC7_COMPRESSION,
		GENERIC_DOWNSAMPLE,
		VIRTUAL_TEXTURE_PAGE_TILING,
		VIRTUAL_TEXTURE_PAGE_MERGING,

		// Test Shader Profile
		TEST_ROOT_SIGNATURE,
		
		COUNT_OR_ERROR
	};
}