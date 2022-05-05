module;
#include <string>

export module Brawler.AppParams;

export namespace Brawler
{
	struct AppParams
	{
		const std::string_view RootDataDirectory;
		const std::string_view RootOutputDirectory;
		const std::uint64_t SwitchBitMask;
	};
}