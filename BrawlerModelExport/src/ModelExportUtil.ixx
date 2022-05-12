module;
#include <assimp/scene.h>

export module Util.ModelExport;

export namespace Brawler
{
	struct AppParams;
}

export namespace Util
{
	namespace ModelExport
	{
		const Brawler::AppParams& GetLaunchParameters();
	}
}