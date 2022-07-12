#include "NumericLimits.hlsli"

namespace BrawlerHLSL
{
	namespace GPUSceneLimits
	{
		// This is the highest number of material definitions supported by the Brawler Engine.
		static const uint MAX_MATERIAL_DEFINITIONS = (1 << 25);
		
		static const 
	
		static const uint MAX_VERTEX_BUFFER_ELEMENTS = (1 << 22);
		static const uint MAX_INDEX_BUFFER_ELEMENTS = NumericLimits<uint>::MAX;
	}

	struct GPUSceneState
	{
		uint ModelInstanceCount;
		uint3 __Pad0;
	};
}