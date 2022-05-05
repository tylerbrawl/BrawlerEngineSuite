module;
#include <cstdint>

module Brawler.JobCounter;

namespace Brawler
{
	JobCounter::JobCounter() :
		mCounter()
	{}

	void JobCounter::DecrementCounter()
	{
		--mCounter;
	}

	bool JobCounter::IsFinished() const
	{
		return (!mCounter.load());
	}

	void JobCounter::SetCounterValue(std::uint32_t jobCount)
	{
		mCounter.store(jobCount);
	}
}