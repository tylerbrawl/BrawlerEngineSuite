module;
#include "Win32Def.h"

export module Util.Engine;

export namespace Brawler
{
	class AssetManager;
}

export namespace Util
{
	namespace Engine
	{
		Brawler::AssetManager& GetAssetManager();
		IXAudio2& GetXAudio2Engine();
	}
}