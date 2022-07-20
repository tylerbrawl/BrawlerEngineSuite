module;
#include <cstdint>

export module Brawler.VirtualTextureLogicalPage;

export namespace Brawler
{
	struct VirtualTextureLogicalPage
	{
		std::uint32_t LogicalMipLevel;
		std::uint32_t LogicalPageXCoordinate;
		std::uint32_t LogicalPageYCoordinate;
	};
}