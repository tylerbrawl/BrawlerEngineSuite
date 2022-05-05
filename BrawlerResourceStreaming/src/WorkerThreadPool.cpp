module;
#include <cassert>
#include <cstdint>
#include <memory>
#include <thread>
#include <atomic>
#include <exception>

module Brawler.WorkerThreadPool;
import Util.Threading;
import Brawler.Application;
import Brawler.WorkerThread;
import Brawler.ThreadSafeQueue;

namespace Brawler
{
	WorkerThreadPool::WorkerThreadPool(std::uint32_t numWorkerThreads) :
		mThreadArr(),
		mThreadMap(),
		mMainThreadInfo(std::this_thread::get_id()),
		mInitialized(false),
		mExceptionQueue()
	{
		mThreadArr.reserve(numWorkerThreads);

		// First, lock the main thread to its own CPU core.
		Util::Threading::LockCurrentThreadToUnassignedCore();

		// Now, we can create the other worker threads. They will each be atomically assigned
		// their own core automatically.
		for (std::uint32_t i = 0; i < numWorkerThreads; ++i)
		{
			mThreadArr.push_back(std::make_unique<Brawler::WorkerThread>(*this));
			mThreadMap[mThreadArr[i]->GetThreadID()] = mThreadArr[i].get();
		}
	}

	WorkerThreadPool::~WorkerThreadPool()
	{
		for (auto& thread : mThreadArr)
		{
			thread->KillThread();
			thread->Join();
		}
	}

	void WorkerThreadPool::DispatchJob(Job&& job)
	{
		static std::atomic<std::size_t> workerThreadIndex = 0;

		// We do not want the current thread to submit a job to its own queue if it is a worker thread.
		std::size_t currIndex = 0;
		while (!workerThreadIndex.compare_exchange_weak(currIndex, (currIndex + 1) % mThreadArr.size()) || mThreadArr[currIndex]->GetThreadID() == std::this_thread::get_id());

		mThreadArr[currIndex]->AddJob(std::move(job));
	}

	bool WorkerThreadPool::IsInitialized() const
	{
		return mInitialized.load();
	}

	void WorkerThreadPool::SetInitialized()
	{
		// Initialize the ThreadLocalResources for the main thread.
		Util::Threading::GetThreadLocalResources().Initialize();
		
		mInitialized.store(true);
	}

	std::thread::id WorkerThreadPool::GetMainThreadID() const
	{
		return mMainThreadInfo.ThreadID;
	}

	std::size_t WorkerThreadPool::GetWorkerThreadCount() const
	{
		return mThreadArr.size();
	}

	std::optional<Job> WorkerThreadPool::AcquireQueuedJob()
	{
		// First, try to steal jobs from our own queue, if applicable. If we are on
		// the main thread, however, then we can use this time to check for exceptions.
		if (Util::Threading::IsMainThread())
			HandleWorkerThreadExceptions();
		else
		{
			WorkerThread* currThread{ GetWorkerThread(std::this_thread::get_id()) };
			std::optional<Job> queuedJob{ currThread->GetNextJob() };

			if (queuedJob.has_value())
				return queuedJob;
		}

		// Now, try to steal jobs from other queues. Even if the current thread is a
		// worker thread, we will double check its own queue, since there is a race
		// condition present. (The race condition is not harmful because worker threads
		// continuously call WorkerThreadPool::AcquireQueuedJob() and yield as necessary. 
		// Thus, if a different thread does add a job to its queue, it will find it
		// eventually.)
		for (std::size_t i = 0; i < GetWorkerThreadCount(); ++i)
		{
			WorkerThread& victimThread{ *mThreadArr[i] };
			std::optional<Job> queuedJob{ victimThread.GetNextJob() };

			if (queuedJob.has_value())
				return queuedJob;
		}

		return std::optional<Job>{};
	}

	void WorkerThreadPool::ThrowException(std::exception_ptr&& ePtr)
	{
		if (Util::Threading::IsMainThread())
			std::rethrow_exception(ePtr);
		else
			while (!mExceptionQueue.PushBack(std::move(ePtr)));
	}

	void WorkerThreadPool::HandleWorkerThreadExceptions()
	{
		if (!Util::Threading::IsMainThread()) [[unlikely]]
			return;

		std::optional<std::exception_ptr> ePtr{ mExceptionQueue.TryPop() };
		while (ePtr.has_value())
		{
			std::rethrow_exception(*ePtr);
			ePtr = mExceptionQueue.TryPop();
		}
	}

	WorkerThread* WorkerThreadPool::GetWorkerThread(std::thread::id threadID)
	{
		return mThreadMap[threadID];
	}

	const WorkerThread* WorkerThreadPool::GetWorkerThread(std::thread::id threadID) const
	{
		return mThreadMap.at(threadID);
	}
}