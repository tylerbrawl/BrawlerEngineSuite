#pragma once

namespace BrawlerHLSL
{
	enum class LightType
	{
		POINT_LIGHT = 0,
		SPOTLIGHT = 1
	};
		
	struct PackedLightDescriptor
	{
		uint LightBufferIndex : 24;
		uint TypeID : 7;
		uint IsValid : 1;
	};
	
	struct LightDescriptor
	{
		uint LightBufferIndex;
		LightType TypeID;
		bool IsValid;
	};
}