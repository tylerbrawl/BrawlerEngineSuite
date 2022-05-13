module;
#include <assimp/scene.h>

export module Util.ModelExport;
import Brawler.AppParams;

export namespace Util
{
	namespace ModelExport
	{
		const Brawler::AppParams& GetLaunchParameters();
	}
}