module;
#include <cstdint>

export module Brawler.LightID;

export namespace Brawler
{
	enum class LightID : std::uint32_t
	{
		POINT_LIGHT,
		SPOTLIGHT,

		COUNT_OR_ERROR
	};
}