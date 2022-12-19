#include "NumericLimits.hlsli"

namespace BrawlerHLSL
{
	namespace GPUSceneLimits
	{
		// This is the highest number of bindless SRVs which can be active at once in the
		// Brawler Engine.
		static const uint MAX_BINDLESS_SRVS = 500000;
		
		// This is the highest number of material definitions supported by the Brawler Engine.
		static const uint MAX_MATERIAL_DEFINITIONS = (1 << 22);
		
		static const uint MAX_MODEL_INSTANCES = (1 << 16);
		
		static const uint MAX_MODELS = (1 << 15);
		
		static const uint MAX_VIEWS = (1 << 12);
	
		static const uint MAX_VERTEX_BUFFER_ELEMENTS = (1 << 22);
		
		// Until some form of light culling is implemented, MAX_LIGHTS is intentionally kept to
		// a pathetically low value.
		static const uint MAX_LIGHTS = (1 << 8);
	}
}