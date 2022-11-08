namespace BrawlerHLSL
{
	struct MaterialDescriptor
	{
		uint BaseColorTextureSRVIndex;
		uint NormalMapSRVIndex;
		uint RoughnessTextureSRVIndex;
		
		// TODO: This should probably be moved into a separate channel of a
		// different texture.
		uint MetallicTextureSRVIndex;
	};
}