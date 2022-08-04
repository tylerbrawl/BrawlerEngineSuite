module;
#include <cstdint>

export module Brawler.VirtualTextureFeedback;

export namespace Brawler
{
	struct VirtualTextureFeedback
	{
		std::uint32_t VirtualTextureID;

		std::uint8_t LogicalMipLevel;
		std::uint8_t LogicalPageXCoordinate;
		std::uint8_t LogicalPageYCoordinate;
		std::uint8_t __Pad0;
	};
}