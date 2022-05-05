module;
#include <filesystem>

export module Brawler.AssetCompilerContext;
import Brawler.PackerSettings;

export namespace Brawler
{
	struct AssetCompilerContext
	{
		PackerSettings::BuildMode BuildMode;
		std::filesystem::path RootDataDirectory;
		std::filesystem::path RootOutputDirectory;
	};
}