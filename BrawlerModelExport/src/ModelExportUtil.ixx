module;
#include <assimp/scene.h>

export module Util.ModelExport;
import Brawler.LaunchParams;

export namespace Util
{
	namespace ModelExport
	{
		const Brawler::LaunchParams& GetLaunchParameters();
	}
}