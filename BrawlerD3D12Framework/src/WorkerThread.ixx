module;
#include <thread>
#include <atomic>

export module Brawler.WorkerThread;
import Brawler.Job;
import Brawler.ThreadLocalResources;
import Util.Threading;

export namespace Brawler
{
	class WorkerThreadPool;

	class WorkerThread
	{
	private:
		friend ThreadLocalResources& Util::Threading::GetThreadLocalResources();

	public:
		WorkerThread(WorkerThreadPool& threadPool, const std::uint32_t threadIndex);

		WorkerThread(const WorkerThread& rhs) = delete;
		WorkerThread& operator=(const WorkerThread& rhs) = delete;

		WorkerThread(WorkerThread&& rhs) noexcept = default;
		WorkerThread& operator=(WorkerThread&& rhs) noexcept = default;

		std::thread::id GetThreadID() const;
		void KillThread();
		void Join();

	private:
		void Initialize();
		void ExecuteMainLoop();

		bool IsWorkerThreadPoolWaitAcceptable() const;
		void OnWorkerThreadPoolWaitDenied();

	private:
		std::thread mThread;
		WorkerThreadPool* mPool;
		ThreadLocalResources mResources;
		std::atomic<bool> mKeepGoing;
	};
}