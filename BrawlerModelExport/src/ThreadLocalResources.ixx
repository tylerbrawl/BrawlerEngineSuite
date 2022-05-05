module;
#include <array>

export module Brawler.ThreadLocalResources;
import Brawler.ThreadSafeQueue;
import Brawler.JobPriority;
import Brawler.Job;
import Util.General;

export namespace Brawler
{
	// Add data which each WorkerThread should keep its own version of to this structure.

	struct ThreadLocalResources
	{
		// This struct is currently empty.
	};
}