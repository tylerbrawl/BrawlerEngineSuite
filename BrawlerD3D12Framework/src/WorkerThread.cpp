module;
#include <thread>
#include <exception>

module Brawler.WorkerThread;
import Util.Threading;
import Brawler.JobPriority;
import Brawler.WorkerThreadPool;
import Util.General;
import Util.Coroutine;

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

				if (!jobExecuted) [[likely]]
					mPool->WaitForJobDispatch(previousJobQueueNotifierValue);
			}
			catch (...)
			{
				mPool->ReThrowException(std::current_exception());
				KillThread();
			}
		}
	}
}