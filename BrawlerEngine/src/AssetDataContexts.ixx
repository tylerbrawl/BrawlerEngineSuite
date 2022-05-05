module;
#include <cstddef>

export module Brawler.AssetDataContexts;

export namespace Brawler
{
	struct AssetDataLoadContext
	{
		std::size_t NumBytesToLoad;
	};

	struct AssetDataUnloadContext
	{
		std::size_t NumBytesToUnload;
	};
}