module;
#include <assimp/scene.h>

module Util.ModelExport;
import Brawler.Application;

namespace Util
{
	namespace ModelExport
	{
		const Brawler::LaunchParams& GetLaunchParameters()
		{
			thread_local const Brawler::LaunchParams& launchParams{ Brawler::GetApplication().GetLaunchParameters() };
			return launchParams;
		}
	}
}