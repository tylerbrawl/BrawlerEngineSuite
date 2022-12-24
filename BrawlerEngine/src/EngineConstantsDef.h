#pragma once
#include <compare>

import Brawler.NZStringView;

// I really did want to make this a module. I really did. However, an MSVC internal compiler error (ICE)
// prevents me from reliably doing so in practice at the time of writing this. I filed a bug report at
// https://developercommunity.visualstudio.com/t/Initialization-of-static-const-std::file/10062952.

namespace Brawler
{
	namespace D3D12
	{
		// Name: PSO_CACHE_FILE_NAME
		//
		// Description: This is the name of the file which will be used as the PSO library for this
		// application. It should be different for each application.
		constexpr Brawler::NZWStringView PSO_CACHE_FILE_NAME{ L"Brawler Engine PSO Cache.bpl" };
	}
}
