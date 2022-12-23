#pragma once

namespace BrawlerHLSL
{
	struct GlobalTextureDescription
	{
		uint BindlessIndex;
		uint GlobalTextureDimensions;
		uint PaddedPageDimensions;
		uint __Pad0;
	};
}