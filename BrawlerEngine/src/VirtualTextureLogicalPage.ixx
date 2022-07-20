module;
#include <cstdint>

export module Brawler.VirtualTextureLogicalPage;
import Brawler.VirtualTexture;

export namespace Brawler
{
	struct VirtualTextureLogicalPage
	{
		VirtualTexture* VirtualTexturePtr;
		std::uint32_t LogicalMipLevel;
		std::uint32_t LogicalPageXCoordinate;
		std::uint32_t LogicalPageYCoordinate;
	};
}