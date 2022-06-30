module;
#include <atomic>
#include <vector>

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
		JobCounter();

		bool IsFinished() const;
		void DecrementCounter();

	private:
		void SetCounterValue(std::uint32_t jobCount);

		void NotifyThreadEntry();
		void NotifyThreadExit();

	private:
		std::atomic<std::uint32_t> mCounter;
		std::vector<std::uint32_t> mThreadEntryCountArr;
	};
}