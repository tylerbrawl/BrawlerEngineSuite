module;

export module Brawler.JobCounterGuard;
import Brawler.JobCounter;

export namespace Brawler
{
	class JobCounterGuard
	{
	public:
		explicit JobCounterGuard(JobCounter* counter);

		~JobCounterGuard();

		JobCounterGuard(const JobCounterGuard& rhs) = delete;
		JobCounterGuard& operator=(const JobCounterGuard& rhs) = delete;

		JobCounterGuard(JobCounterGuard&& rhs) noexcept = delete;
		JobCounterGuard& operator=(JobCounterGuard&& rhs) noexcept = delete;

	private:
		JobCounter* mCounterPtr;
	};
}