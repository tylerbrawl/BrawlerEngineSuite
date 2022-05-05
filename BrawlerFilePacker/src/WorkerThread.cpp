module;
#include <thread>

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

			mResources.Initialize();

			ExecuteMainLoop();
		} };

		while (!isThreadInitialized.load())
			std::this_thread::yield();
	}

	void WorkerThread::AddJob(Job&& job)
	{
		while (!(mResources.JobQueueMap[Util::General::EnumCast(job.GetPriority())].PushBack(std::move(job))));
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

	std::optional<Job> WorkerThread::GetNextJob()
	{
		// Search the queues in order from most critical to least critical.

		for (std::underlying_type_t<JobPriority> i = (static_cast<std::underlying_type_t<JobPriority>>(JobPriority::COUNT) - 1); i >= 0; --i)
		{
			std::optional<Job> job{ mResources.JobQueueMap[i].TryPop() };

			if (job.has_value())
				return job;
		}

		return std::optional<Job>{};
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
				mPool->ThrowException(std::current_exception());
				mKeepGoing.store(false);
			}
		}
	}
}