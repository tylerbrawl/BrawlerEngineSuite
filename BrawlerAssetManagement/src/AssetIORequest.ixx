module;
#include <span>

export module Brawler.AssetManagement.AssetIORequest;
import Brawler.FilePathHash;

export namespace Brawler
{
	namespace AssetManagement
	{
		struct AssetIORequest
		{
			/// <summary>
			/// This is the FilePathHash referring to the file which is to be loaded
			/// as part of asset initialization. It should correspond to an entry in
			/// the main BPK data file.
			/// </summary>
			Brawler::FilePathHash PathHash;

			/// <summary>
			/// This represents the area in memory to which the loaded asset data will
			/// be written.
			/// </summary>
			std::span<std::byte> DestDataSpan;
		};
	}
}