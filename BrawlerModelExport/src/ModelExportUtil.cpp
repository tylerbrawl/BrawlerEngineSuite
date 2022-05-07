module;
#include <assimp/scene.h>

module Util.ModelExport;
import Brawler.Application;
import Brawler.SceneLoader;

namespace Util
{
	namespace ModelExport
	{
		const aiScene& GetScene()
		{
			thread_local const aiScene& scene{ Brawler::GetApplication().GetSceneLoader().GetScene() };
			return scene;
		}

		const Brawler::AppParams& GetLaunchParameters()
		{
			thread_local const Brawler::AppParams& appParams{ Brawler::GetApplication().GetLaunchParameters() };
			return appParams;
		}
	}
}