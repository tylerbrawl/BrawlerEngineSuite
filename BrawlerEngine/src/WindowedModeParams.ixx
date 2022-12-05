module;

export module Brawler.WindowedModeParams;
import Brawler.Math.MathTypes;

export namespace Brawler
{
	struct WindowedModeParams
	{
		Math::Int2 WindowOriginCoordinates;
		Math::UInt2 WindowSize;
	};
}