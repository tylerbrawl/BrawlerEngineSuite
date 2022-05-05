module;
#include <thread>
#include "Win32Def.h"

module Util.Engine;
import Brawler.Application;
import Brawler.AudioManager;

namespace Util
{
	namespace Engine
	{
		Brawler::AssetManager& GetAssetManager()
		{
			thread_local Brawler::AssetManager& assetManager{ Brawler::Application::GetInstance().GetAssetManager() };

			return assetManager;
		}

		IXAudio2& GetXAudio2Engine()
		{
			thread_local IXAudio2& xAudio2Engine{ Brawler::Application::GetInstance().GetAudioManager().GetXAudio2Engine() };

			return xAudio2Engine;
		}
	}
}