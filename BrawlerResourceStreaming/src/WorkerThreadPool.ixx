module;
#include <vector>
#include <optional>
#include <exception>
#include <thread>
#include <unordered_map>

export module Brawler.WorkerThreadPool;
import Brawler.WorkerThread;
import Brawler.Job;
import Brawler.ThreadLocalResources;
import Util.Threading;
import Brawler.ThreadSafeQueue;

namespace
{
	static constexpr std::size_t EXCEPTION_QUEUE_SIZE = 64;
}

export namespace Brawler
{
	class WorkerThreadPool
	{
	private:
		struct MainThreadInfo
		{
			std::thread::id ThreadID;
			ThreadLocalResources Resources;

			explicit MainThreadInfo(std::thread::id threadID) :
				ThreadID(threadID),
				Resources()
			{}
		};

	private:
		friend WorkerThread* Util::Threading::GetCurrentWorkerThread();
		friend ThreadLocalResources& Util::Threading::GetThreadLocalResources();

	public:
		explicit WorkerThreadPool(std::uint32_t numWorkerThreads = (std::thread::hardware_concurrency() - 1));
		~WorkerThreadPool();

		WorkerThreadPool(const WorkerThreadPool& rhs) = delete;
		WorkerThreadPool& operator=(const WorkerThreadPool& rhs) = delete;

		WorkerThreadPool(WorkerThreadPool&& rhs) noexcept = default;
		WorkerThreadPool& operator=(WorkerThreadPool&& rhs) noexcept = default;

		void DispatchJob(Job&& job);
		bool IsInitialized() const;
		void SetInitialized();

		// Returns the thread ID of the main thread.
		std::thread::id GetMainThreadID() const;

		// Returns the number of worker threads in the pool.
		std::size_t GetWorkerThreadCount() const;

		// If the current thread is a worker thread, then this function first tries to pull
		// a job from its own queue. If that fails, or if the current thread is the main
		// thread, then we attempt to steal jobs from other worker threads.
		std::optional<Job> AcquireQueuedJob();

		void ThrowException(std::exception_ptr&& ePtr);

		/// <summary>
		/// Call this function from the main thread to handle exceptions thrown by WorkerThreads
		/// which they themselves could not handle.
		/// </summary>
		void HandleWorkerThreadExceptions();

	private:
		WorkerThread* GetWorkerThread(std::thread::id threadID);
		const WorkerThread* GetWorkerThread(std::thread::id threadID) const;

	private:
		std::vector<std::unique_ptr<WorkerThread>> mThreadArr;
		std::unordered_map<std::thread::id, WorkerThread*> mThreadMap;
		MainThreadInfo mMainThreadInfo;
		std::atomic<bool> mInitialized;
		ThreadSafeQueue<std::exception_ptr, EXCEPTION_QUEUE_SIZE> mExceptionQueue;
	};
}