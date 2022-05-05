module;
#include <string>

export module Brawler.AppParams;

export namespace Brawler
{
	struct AppParams
	{
		std::wstring InputMeshFilePath;
		std::wstring RootOutputDirectory;
	};
}