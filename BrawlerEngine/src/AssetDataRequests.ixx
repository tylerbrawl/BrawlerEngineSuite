module;
#include <cstddef>

export module Brawler.AssetDataRequests;

export namespace Brawler
{
	class I_Asset;
}

export namespace Brawler
{
	struct AssetDataLoadRequest
	{
		/// <summary>
		/// This is a reference to the I_Asset which is requesting to load some
		/// data.
		/// </summary>
		I_Asset& Asset;

		/// <summary>
		/// This is the size, in bytes, of the data which will be loaded when
		/// this request is fulfilled.
		/// </summary>
		std::size_t NumBytesToLoad;
	};

	struct AssetDataUnloadRequest
	{
		/// <summary>
		/// This is a reference to the I_Asset which is requesting to unload some
		/// data.
		/// </summary>
		I_Asset& Asset;

		/// <summary>
		/// This is the size, in bytes, of the data which will be unloaded when
		/// this request is fulfilled.
		/// </summary>
		std::size_t NumBytesToUnload;
	};
}