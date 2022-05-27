module;
#include <vector>
#include <functional>

module Brawler.DelayedJobGroup;
import Util.Threading;
import Brawler.ThreadLocalResources;
import Brawler.DelayedJobSubmitter;

namespace Brawler
{
	DelayedJobGroup::DelayedJobGroup(const JobPriority priority) :
		mJobArr(),
		mPriority(priority)
	{}

	void DelayedJobGroup::AddJob(std::function<void()>&& callback)
	{
		mJobArr.push_back(Job{ std::move(callback), nullptr, mPriority });
	}

	void DelayedJobGroup::Reserve(const std::size_t jobCount)
	{
		mJobArr.reserve(jobCount);
	}

	void DelayedJobGroup::SubmitDelayedJobs(Win32::SafeHandle&& hEvent)
	{
		Util::Threading::GetThreadLocalResources().GetDelayedJobSubmitter().AddDelayedJobSubmission(DelayedJobSubmitter::DelayedJobSubmissionInfo{
			.DelayedJobArr{ std::move(mJobArr) },
			.HEvent{ std::move(hEvent) }
		});
	}
}