module;
#include <vector>
#include <functional>

export module Brawler.DelayedJobGroup;
import Brawler.Job;
import Brawler.JobPriority;
import Brawler.Win32.SafeHandle;

export namespace Brawler
{
	class DelayedJobGroup
	{
	public:
		explicit DelayedJobGroup(const JobPriority priority = JobPriority::NORMAL);

		DelayedJobGroup(const DelayedJobGroup& rhs) = delete;
		DelayedJobGroup& operator=(const DelayedJobGroup& rhs) = delete;

		DelayedJobGroup(DelayedJobGroup&& rhs) noexcept = default;
		DelayedJobGroup& operator=(DelayedJobGroup&& rhs) noexcept = default;

		void AddJob(std::function<void()>&& callback);
		void Reserve(const std::size_t jobCount);

		void SubmitDelayedJobs(Win32::SafeHandle&& hEvent);

	private:
		std::vector<Job> mJobArr;
		JobPriority mPriority;
	};
}