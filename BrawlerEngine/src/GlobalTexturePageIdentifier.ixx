module;
#include <cstdint>

export module Brawler.GlobalTexturePageIdentifier;

export namespace Brawler
{
	struct alignas(std::uint32_t) GlobalTexturePageIdentifier
	{
		std::uint8_t GlobalTextureID;
		std::uint8_t GlobalTexturePageXCoordinate;
		std::uint8_t GlobalTexturePageYCoordinate;
	};
}