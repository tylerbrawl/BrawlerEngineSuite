module;
#include <exception>
#include <optional>
#include <thread>

module Util.Coroutine;
import Brawler.Job;
import Brawler.Application;
import Brawler.WorkerThreadPool;

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

		void TryExecuteJob()
		{
			thread_local Brawler::WorkerThreadPool& threadPool{ Brawler::Application::GetInstance().GetWorkerThreadPool() };
			std::optional<Brawler::Job> stolenJob{ threadPool.AcquireQueuedJob() };

			if (stolenJob.has_value())
				stolenJob->Execute();
			else
				std::this_thread::yield();
		}
	}
}