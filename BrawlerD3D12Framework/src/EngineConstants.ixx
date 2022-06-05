module;
#include <compare>

export module Brawler.EngineConstants;
import :EngineConstantsDef;
import Brawler.NZStringView;

/*
The idea behind EngineConstants is that each application which uses the BrawlerD3D12Framework will
have a set of constant values which differ from other applications using the framework. For instance, the
PSO cache for one program should be stored in a different directory than that used by another program.

To that end, projects which reference the BrawlerD3D12Framework should define their own module partition
unit of Brawler.EngineConstants called EngineConstantsDef. This module should contain a consteval definition
for every function mentioned in this module interface unit.

The build system of the project should also be configured to create a symbolic link to the file which
implements this module partition unit called EngineConstantsDef.ixx. A similar solution is used to link
the PSODefinitions of a given project to the BrawlerD3D12Framework.

We use this method in order to guarantee that the constants in question are usable in a constant-evaluated
context. I would love it if there were a better solution to this, but as of right now, this is the best
thing that I can think of.
*/

export namespace Brawler
{
	namespace D3D12
	{
		// Name: PSO_CACHE_FILE_NAME
		//
		// Description: This is the name of the file which will be used as the PSO library for this
		// application. It should be different for each application.
		constexpr Brawler::NZWStringView PSO_CACHE_FILE_NAME{ GetPSOCacheFileName() };
	}
}