#include "NumericLimits.hlsli"

namespace BrawlerHLSL
{
	namespace GPUSceneLimits
	{
		// This is the highest number of material definitions supported by the Brawler Engine.
		static const uint MAX_MATERIAL_DEFINITIONS = (1 << 22);
		
		static const uint MAX_VIRTUAL_TEXTURE_DESCRIPTIONS = 500000;
		
		static const uint MAX_MODEL_INSTANCES = (1 << 16);
		
		static const uint MAX_VIEWS = (1 << 12);
	
		static const uint MAX_VERTEX_BUFFER_ELEMENTS = (1 << 22);
		static const uint MAX_INDEX_BUFFER_ELEMENTS = (1 << 22);
		
		// Until some form of light culling is implemented, MAX_LIGHTS is intentionally kept to
		// a pathetically low value.
		static const uint MAX_LIGHTS = 256;
	}
}