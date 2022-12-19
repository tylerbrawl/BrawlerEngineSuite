#include "GPUSceneLimits.hlsli"

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
		
		inline bool HasBaseColorTexture()
		{
			return (BaseColorTextureSRVIndex < GPUSceneLimits::MAX_BINDLESS_SRVS);
		}
		
		inline bool HasNormalMap()
		{
			return (NormalMapSRVIndex < GPUSceneLimits::MAX_BINDLESS_SRVS);
		}
		
		inline bool HasRoughnessMap()
		{
			return (RoughnessTextureSRVIndex < GPUSceneLimits::MAX_BINDLESS_SRVS);
		}
		
		inline bool HasMetallicMap()
		{
			return (MetallicTextureSRVIndex < GPUSceneLimits::MAX_BINDLESS_SRVS);
		}
	};
}