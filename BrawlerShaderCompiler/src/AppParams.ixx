module;
#include <compare>  // Classic MSVC modules jank...
#include <filesystem>

export module Brawler.AppParams;
import Brawler.ShaderProfileID;

export namespace Brawler
{
	struct AppParams
	{
		std::filesystem::path RootSourceDirectory;
		ShaderProfiles::ShaderProfileID ShaderProfile;
	};
}