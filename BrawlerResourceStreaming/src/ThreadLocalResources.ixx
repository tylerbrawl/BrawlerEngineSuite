module;
#include <array>

export module Brawler.ThreadLocalResources;
import Brawler.ThreadSafeQueue;
import Brawler.JobPriority;
import Brawler.Job;

namespace
{
	static constexpr std::size_t JOB_QUEUE_SIZE = 256;
}

export namespace Brawler
{
	// Add data which each WorkerThread should keep its own version of to this structure.

	struct ThreadLocalResources
	{
		// The queues within the JobQueueMap are the only thread-safe types in the
		// ThreadLocalResources structure. (The map itself is not thread-safe, but
		// is not modified after the thread is initialized.)
		std::array<ThreadSafeQueue<Job, JOB_QUEUE_SIZE>, std::to_underlying(JobPriority::COUNT)> JobQueueMap;

		ThreadLocalResources() :
			JobQueueMap()
		{}

		void Initialize()
		{}
	};
}