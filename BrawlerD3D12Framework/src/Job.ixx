module;
#include <functional>

export module Brawler.Job;
import Brawler.JobCounter;
import Brawler.JobPriority;

export namespace Brawler
{
	class Job
	{
	public:
		Job() = default;
		Job(std::function<void()>&& callback, std::shared_ptr<JobCounter> counterPtr, JobPriority priority = JobPriority::NORMAL);

		void Execute();
		JobPriority GetPriority() const;

	private:
		std::function<void()> mCallback;
		std::shared_ptr<JobCounter> mCounterPtr;
		JobPriority mPriority;
		std::uint64_t mCachedFrameNumber;
	};
}