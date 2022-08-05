module;
#include <cstdint>

export module Brawler.VirtualTextureFeedback;

export namespace Brawler
{
	struct VirtualTextureFeedback
	{
		std::uint32_t VirtualTextureID;
		std::uint32_t LogicalMipLevel;
		std::uint32_t LogicalPageXCoordinate;
		std::uint32_t LogicalPageYCoordinate;
	};
}