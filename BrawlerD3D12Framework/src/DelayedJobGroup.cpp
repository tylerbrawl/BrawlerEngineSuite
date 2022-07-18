module;
#include <vector>
#include <functional>
#include <cassert>

module Brawler.DelayedJobGroup;

namespace Brawler
{
	DelayedJobGroup::DelayedJobGroup(const JobPriority priority) :
		mJobArr(),
		mPriority(priority)
	{}

	void DelayedJobGroup::AddJob(std::move_only_function<void()>&& callback)
	{
		assert(callback && "ERROR: A std::move_only_function<void()> which was never assigned a callback was provided to a DelayedJobGroup!");
		mJobArr.push_back(Job{ std::move(callback), nullptr, mPriority });
	}

	void DelayedJobGroup::Reserve(const std::size_t jobCount)
	{
		mJobArr.reserve(jobCount);
	}
}