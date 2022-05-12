module;
#include <assimp/scene.h>

module Util.ModelExport;
import Brawler.Application;
import Brawler.SceneLoader;

namespace Util
{
	namespace ModelExport
	{
		const Brawler::AppParams& GetLaunchParameters()
		{
			thread_local const Brawler::AppParams& appParams{ Brawler::GetApplication().GetLaunchParameters() };
			return appParams;
		}
	}
}