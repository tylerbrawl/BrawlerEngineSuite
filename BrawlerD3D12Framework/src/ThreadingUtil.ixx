module;
#include <cstddef>

export module Util.Threading;
import Brawler.ThreadLocalResources;

export namespace Brawler
{
	class WorkerThread;
}

export namespace Util
{
	namespace Threading
	{
		// Sets the affinity of the current thread to a unique logical CPU core. The function
		// assumes that it will only be called at most once for each core, and does no error
		// checking to verify this.
		//
		// NOTE: This function is processor group-aware (see 
		// https://docs.microsoft.com/en-us/windows/win32/procthread/processor-groups).
		void LockCurrentThreadToUnassignedCore();

		// Returns true if the calling thread is the main thread (i.e., not a WorkerThread)
		// and false otherwise.
		bool IsMainThread();

		// If the current thread is a worker thread, then the returned value is defined, and
		// it is a pointer to the relevant worker thread instance; otherwise, the value is
		// nullptr.
		Brawler::WorkerThread* GetCurrentWorkerThread();

		// Returns the ThreadLocalResources instance of the current thread. ThreadLocalResources
		// are generally *NOT* synchronized, and should be accessed with extreme care from
		// other threads.
		Brawler::ThreadLocalResources& GetThreadLocalResources();

		std::size_t GetWorkerThreadCount();
		std::size_t GetTotalThreadCount();
	}
}