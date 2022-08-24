module;
#include <cstdint>

export module Brawler.ActiveGlobalTexturePageStats;

export namespace Brawler
{
	struct ActiveGlobalTexturePageStats
	{
		std::uint32_t NumPagesFilled;
		std::uint32_t NumPagesFilledForCombinedPages;
	};
}