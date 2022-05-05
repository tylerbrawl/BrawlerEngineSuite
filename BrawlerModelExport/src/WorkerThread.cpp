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
			try
			{
				Util::Coroutine::TryExecuteJob();
			}
			catch (...)
			{
				Brawler::GetApplication().GetWorkerThreadPool().ReThrowException(std::current_exception());
				KillThread();
			}
		}
	}
}