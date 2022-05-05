module;
#include <string>

export module Brawler.SerializationContext;

export namespace Brawler
{
	class SceneLoader;
}

export namespace Brawler
{
	struct SerializationContext
	{
		SceneLoader& LoadedScene;
		std::wstring_view InputMeshFilePath;
		std::wstring_view RootOutputDirectory;
	};
}