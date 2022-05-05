module;
#include <vector>
#include <optional>
#include <thread>
#include <unordered_map>
#include <exception>

export module Brawler.WorkerThreadPool;
import Brawler.WorkerThread;
import Brawler.Job;
import Brawler.JobPriority;
import Brawler.ThreadLocalResources;
import Util.Threading;
import Brawler.ThreadSafeQueue;

namespace Brawler
{
	namespace IMPL
	{
		static constexpr std::size_t JOB_QUEUE_SIZE = 2048;
		static constexpr std::size_t EXCEPTION_QUEUE_SIZE = 16;
	}
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

		/// <summary>
		/// Attempts to retrieve a CPU job from one of the job queues in this
		/// WorkerThreadPool instance. The queues are searched in the order of decreasing
		/// priority.
		/// </summary>
		/// <returns>
		/// If a CPU job was extracted from one of the queues, then the returned
		/// std::optional instance contains the extracted job. Otherwise, the returned
		/// std::optional instance is empty.
		/// </returns>
		std::optional<Job> AcquireQueuedJob();

		/// <summary>
		/// This function is called by the WorkerThreads when they come across an uncaught
		/// exception. Its purpose is to send them to the WorkerThreadPool's exception pointer
		/// queue so that the main thread can find them and exit the program gracefully.
		/// </summary>
		/// <param name="exceptionPtr">
		/// - The uncaught exception which was generated on a WorkerThread.
		/// </param>
		void ReThrowException(std::exception_ptr exceptionPtr);

	private:
		WorkerThread* GetWorkerThread(std::thread::id threadID);
		const WorkerThread* GetWorkerThread(std::thread::id threadID) const;

		/// <summary>
		/// This function is called by the main thread periodically to check for any uncaught
		/// exceptions which the WorkerThreads encountered.
		/// </summary>
		void HandleThrownExceptions();

	private:
		std::array<ThreadSafeQueue<Brawler::Job, IMPL::JOB_QUEUE_SIZE>, std::to_underlying(JobPriority::COUNT)> mJobQueueArr;
		ThreadSafeQueue<std::exception_ptr, IMPL::EXCEPTION_QUEUE_SIZE> mExceptionPtrQueue;
		std::vector<std::unique_ptr<WorkerThread>> mThreadArr;
		std::unordered_map<std::thread::id, WorkerThread*> mThreadMap;
		MainThreadInfo mMainThreadInfo;
		std::atomic<bool> mInitialized;
	};
}