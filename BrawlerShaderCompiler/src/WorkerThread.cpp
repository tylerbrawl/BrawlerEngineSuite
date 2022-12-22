module;
#include <thread>
#include <exception>

module Brawler.WorkerThread;
import Util.Threading;
import Brawler.JobPriority;
import Brawler.Application;
import Brawler.WorkerThreadPool;
import Util.General;
import Util.Coroutine;
import Util.Win32;

namespace Brawler
{
	WorkerThread::WorkerThread(WorkerThreadPool& threadPool) :
		mThread(),
		mPool(&threadPool),
		mResources(),
		mKeepGoing(true)
	{
		std::atomic<bool> isThreadInitialized = false;

		mThread = std::thread{ [this, &isThreadInitialized, &threadPool]()
		{
			Initialize();
			isThreadInitialized.store(true);

			// Wait until all of the threads in the WorkerThreadPool are
			// initialized to begin execution.
			while (!threadPool.IsInitialized())
				std::this_thread::yield();

			ExecuteMainLoop();
		} };

		while (!isThreadInitialized.load())
			std::this_thread::yield();
	}

	std::thread::id WorkerThread::GetThreadID() const
	{
		return mThread.get_id();
	}

	void WorkerThread::KillThread()
	{
		mKeepGoing.store(false);
	}

	void WorkerThread::Join()
	{
		if (mThread.joinable())
			mThread.join();
	}

	void WorkerThread::Initialize()
	{
		Util::Threading::LockCurrentThreadToUnassignedCore();
	}

	void WorkerThread::ExecuteMainLoop()
	{
		while (mKeepGoing.load())
		{
			// If we get an uncaught exception which gets propagated up to the beginning
			// of the call stack for a WorkerThread instance, then we probably won't be able
			// to continue gracefully. Trust me: It's not worth it to try and recover from
			// it. Doing so is prone to race conditions and incredibly difficult - if not impossible
			// - to get right.
			try
			{
				Util::Coroutine::TryExecuteJob();
			}
			catch (const std::exception& e)
			{
				Util::Win32::WriteFormattedConsoleMessage(e.what(), Util::Win32::ConsoleFormat::CRITICAL_FAILURE);
				std::terminate();
			}
			catch (...)
			{
				Util::Win32::WriteFormattedConsoleMessage(L"ERROR: An unrecoverable error was detected. The program has been terminated.", Util::Win32::ConsoleFormat::CRITICAL_FAILURE);
				std::terminate();
			}
		}
	}
}