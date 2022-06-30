module;
#include <mutex>
#include <unordered_map>
#include <cstdint>
#include <thread>

module Brawler.JobCounter;

namespace Brawler
{
	bool JobCounter::IsFinished() const
	{
		std::scoped_lock<std::mutex> lock{ mCritSection };

		if (!mThreadEntryMap.contains(std::this_thread::get_id()))
			return (mCounter.load() == 0);

		return (mThreadEntryMap.at(std::this_thread::get_id()) > 1 || mCounter.load() == 0);
	}

	void JobCounter::DecrementCounter() noexcept
	{
		--mCounter;
	}

	void JobCounter::SetCounterValue(std::uint32_t jobCount)
	{
		mCounter.store(jobCount);
	}

	void JobCounter::NotifyThreadEntry()
	{
		std::scoped_lock<std::mutex> lock{ mCritSection };
		++(mThreadEntryMap[std::this_thread::get_id()]);
	}

	void JobCounter::NotifyThreadExit()
	{
		std::scoped_lock<std::mutex> lock{ mCritSection };
		--(mThreadEntryMap[std::this_thread::get_id()]);
	}
}