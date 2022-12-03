module;
#include <functional>
#include <memory>

export module Brawler.Job;
import Brawler.JobCounter;
import Brawler.JobPriority;

export namespace Brawler
{
	class Job
	{
	public:
		Job() = default;
		Job(std::move_only_function<void()>&& callback, std::shared_ptr<JobCounter> counterPtr, JobPriority priority = JobPriority::NORMAL);

		Job(const Job& rhs) = delete;
		Job& operator=(const Job& rhs) = delete;

		Job(Job&& rhs) noexcept = default;
		Job& operator=(Job&& rhs) noexcept = default;

		void Execute();
		JobPriority GetPriority() const;

	private:
		std::move_only_function<void()> mCallback;
		std::shared_ptr<JobCounter> mCounterPtr;
		JobPriority mPriority;
		std::uint64_t mCachedFrameNumber;
	};
}