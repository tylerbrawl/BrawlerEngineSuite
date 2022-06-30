module;
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <thread>

export module Brawler.JobCounter;

export namespace Brawler
{
	class JobGroup;
	class JobCounterGuard;
}

export namespace Brawler
{
	class JobCounter
	{
	private:
		friend class JobGroup;
		friend class JobCounterGuard;

	public:
		JobCounter() = default;

		bool IsFinished() const;
		void DecrementCounter() noexcept;

	private:
		void SetCounterValue(std::uint32_t jobCount);

		void NotifyThreadEntry();
		void NotifyThreadExit();

	private:
		std::atomic<std::uint32_t> mCounter;
		std::unordered_map<std::thread::id, std::uint32_t> mThreadEntryMap;
		mutable std::mutex mCritSection;
	};
}