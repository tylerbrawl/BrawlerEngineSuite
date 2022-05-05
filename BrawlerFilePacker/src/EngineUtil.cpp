module;
#include <cstdint>
#include <thread>

module Util.Engine;
import Brawler.Application;
import Brawler.HashProvider;
import Brawler.PackerSettings;

namespace Util
{
	namespace Engine
	{
		Brawler::HashProvider& GetHashProvider()
		{
			thread_local Brawler::HashProvider& hashProvider{ Brawler::Application::GetInstance().GetHashProvider() };

			return hashProvider;
		}

		Brawler::PackerSettings::BuildMode GetAssetBuildMode()
		{
			thread_local const Brawler::PackerSettings::BuildMode buildMode{ Brawler::Application::GetInstance().GetAssetBuildMode() };

			return buildMode;
		}

		std::int32_t GetZSTDCompressionLevel()
		{
			return Brawler::PackerSettings::GetZSTDCompressionLevelForBuildMode(GetAssetBuildMode());
		}
	}
}