module;
#include <cassert>
#include <cstdint>
#include <memory>
#include <thread>
#include <atomic>

module Brawler.WorkerThreadPool;
import Util.Threading;
import Brawler.WorkerThread;

namespace Brawler
{
	WorkerThreadPool::WorkerThreadPool(std::uint32_t numWorkerThreads) :
		mJobQueueArr(),
		mJobQueueNotifier(),
		mThreadArr(),
		mThreadMap(),
		mMainThreadInfo(std::this_thread::get_id()),
		mInitialized(false),
		mActive(true)
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
		mActive.store(false);
		
		// If we throw an exception on the main thread before the WorkerThreadPool::SetInitialized()
		// function is called, then the other threads will remain active until we initialize it.
		mInitialized.store(true);
		
		for (auto& thread : mThreadArr)
			thread->KillThread();

		mJobQueueNotifier.fetch_add(1, std::memory_order::release);
		mJobQueueNotifier.notify_all();

		for (auto& thread : mThreadArr)
			thread->Join();
	}

	void WorkerThreadPool::DispatchJob(Job&& job)
	{
		// If we cannot store the job in the queue, then we must execute it immediately. (Alternatively,
		// we could just do a spin lock for some time to see if space opens up, but wouldn't that time
		// spent waiting be better spent executing actual work?)
		if (!mJobQueueArr[std::to_underlying(job.GetPriority())].PushBack(std::move(job))) [[unlikely]]
			job.Execute();
		else
		{
			// On successfully adding a job to the queue, we can notify the other threads to wake up
			// if they were waiting. We do this instead of having the threads check the queue
			// continuously in order to increase efficiency, as the OS will then have an easier time
			// understanding how to make good use of the application's threads.
			//
			// Linus Torvalds gave a good rant about this very subject for Linux, and I'd be surprised
			// if the same doesn't hold on other operating systems.

			mJobQueueNotifier.fetch_add(1, std::memory_order::relaxed);
			mJobQueueNotifier.notify_one();
		}
	}

	bool WorkerThreadPool::IsInitialized() const
	{
		return mInitialized.load();
	}

	void WorkerThreadPool::SetInitialized()
	{
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
		// We can reasonably expect the main thread to call this function often enough
		// to allow it to handle exceptions thrown by WorkerThreads here.
		if (Util::Threading::IsMainThread())
			HandleThrownExceptions();
		
		// Attempt to acquire jobs from the queues in order of decreasing priority.
		for (std::int32_t i = static_cast<std::int32_t>(JobPriority::COUNT) - 1; i >= 0; --i)
		{
			std::optional<Job> acquiredJob{ mJobQueueArr[i].TryPop() };

			if (acquiredJob.has_value())
				return acquiredJob;
		}
		
		return std::optional<Job>{};
	}

	void WorkerThreadPool::ReThrowException(std::exception_ptr exceptionPtr)
	{
		assert(!Util::Threading::IsMainThread() && "ERROR: WorkerThreadPool::ReThrowException() should only be called by WorkerThreads!");
		
		const bool insertionResult = mExceptionPtrQueue.PushBack(std::move(exceptionPtr));
		assert(insertionResult && "ERROR: Something was so broken that the exception pointer queue in the WorkerThreadPool couldn't hold all of the generated exception pointers!");
	}

	bool WorkerThreadPool::IsActive() const
	{
		return mActive.load();
	}

	std::uint32_t WorkerThreadPool::GetCurrentJobQueueNotifierValue() const
	{
		return mJobQueueNotifier.load(std::memory_order::acquire);
	}

	void WorkerThreadPool::WaitForJobDispatch(const std::uint32_t previousJobQueueNotifierValue) const
	{
		assert(!Util::Threading::IsMainThread() && "ERROR: WorkerThreadPool::WaitForJobDispatch() should only be called by WorkerThreads!");

		mJobQueueNotifier.wait(previousJobQueueNotifierValue, std::memory_order::acquire);
	}

	WorkerThread* WorkerThreadPool::GetWorkerThread(std::thread::id threadID)
	{
		return mThreadMap[threadID];
	}

	const WorkerThread* WorkerThreadPool::GetWorkerThread(std::thread::id threadID) const
	{
		return mThreadMap.at(threadID);
	}

	void WorkerThreadPool::HandleThrownExceptions()
	{
		assert(Util::Threading::IsMainThread() && "ERROR: WorkerThreadPool::HandleThrownExceptions() should only be called by the main thread!");
		
		std::optional<std::exception_ptr> exceptionPtr{ mExceptionPtrQueue.TryPop() };

		while (exceptionPtr.has_value())
		{
			std::rethrow_exception(*exceptionPtr);
			exceptionPtr = mExceptionPtrQueue.TryPop();
		}
	}
}