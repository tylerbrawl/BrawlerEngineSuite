module;
#include <thread>
#include <exception>

module Brawler.WorkerThread;
import Util.Threading;
import Brawler.JobPriority;
import Brawler.WorkerThreadPool;
import Util.General;
import Util.Coroutine;
import Brawler.DelayedJobSubmitter;

namespace Brawler
{
	WorkerThread::WorkerThread(WorkerThreadPool& threadPool, const std::uint32_t threadIndex) :
		mThread(),
		mPool(&threadPool),
		mResources(),
		mKeepGoing(true)
	{
		std::atomic<bool> isThreadInitialized = false;

		mResources.SetThreadIndex(threadIndex);
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
		while (mKeepGoing.load() && mPool->IsActive())
		{
			try
			{
				const std::uint32_t previousJobQueueNotifierValue = mPool->GetCurrentJobQueueNotifierValue();
				const bool jobExecuted = Util::Coroutine::TryExecuteJob();

				if (!jobExecuted && IsWorkerThreadPoolWaitAcceptable()) [[likely]]
					mPool->WaitForJobDispatch(previousJobQueueNotifierValue);
				else [[unlikely]]
					OnWorkerThreadPoolWaitDenied();
			}
			catch (...)
			{
				mPool->ReThrowException(std::current_exception());
				KillThread();
			}
		}
	}

	bool WorkerThread::IsWorkerThreadPoolWaitAcceptable() const
	{
		// The following list of scenarios are undesirable to perform an atomic wait in:

		//   - The WorkerThread has any delayed CPU jobs to check for.
		if (mResources.GetDelayedJobSubmitter().HasDelayedJobsToCheck()) [[unlikely]]
			return false;

		// Add more scenarios here as necessary.

		return true;
	}

	void WorkerThread::OnWorkerThreadPoolWaitDenied()
	{
		// Check for any delayed CPU jobs which can be submitted to the WorkerThreadPool.
		mResources.GetDelayedJobSubmitter().CheckForDelayedJobSubmissions();
	}
}