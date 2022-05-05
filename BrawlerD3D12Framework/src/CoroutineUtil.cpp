module;
#include <exception>
#include <optional>
#include <thread>

module Util.Coroutine;
import Brawler.Job;
import Brawler.WorkerThreadPool;

namespace Brawler
{
	extern WorkerThreadPool& GetWorkerThreadPool();
}

namespace Util
{
	namespace Coroutine
	{
		void RethrowUnhandledException()
		{
			std::exception_ptr ePtr{ std::current_exception() };

			if (ePtr)
				std::rethrow_exception(ePtr);
		}

		bool TryExecuteJob()
		{
			thread_local Brawler::WorkerThreadPool& threadPool{ Brawler::GetWorkerThreadPool() };
			std::optional<Brawler::Job> stolenJob{ threadPool.AcquireQueuedJob() };

			if (stolenJob.has_value())
			{
				stolenJob->Execute();
				return true;
			}
				
			return false;
		}
	}
}