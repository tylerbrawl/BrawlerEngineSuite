module;
#include <DxDef.h>

export module Brawler.AssetManagement.DirectStorageStatusArrayEntry;
import Brawler.JobPriority;

export namespace Brawler
{
	namespace AssetManagement
	{
		struct DirectStorageStatusArrayEntry
		{
			IDStorageStatusArray& StatusArray;
			std::uint32_t StatusArrayEntryIndex;
			Brawler::JobPriority QueuePriority;
		};
	}
}