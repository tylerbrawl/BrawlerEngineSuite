module;
#include <cstdint>
#include <vector>

module Brawler.JobCounter;
import Util.Threading;
import Brawler.ThreadLocalResources;

namespace Brawler
{
	JobCounter::JobCounter() :
		mCounter(),
		mThreadEntryCountArr()
	{
		mThreadEntryCountArr.resize(Util::Threading::GetTotalThreadCount());
	}

	bool JobCounter::IsFinished() const
	{
		// Without additional checks, there is the potential for a deadlock to occur when an exception is
		// thrown in a job. When an uncaught exception is thrown while executing a CPU job, the associated
		// JobCounter instance is decremented, as expected. However, the following conditions will lead to
		// a deadlock:
		//
		//   1. Thread A begins executing a CPU job associated with JobCounter C.
		//   2. Thread A calls Util::Coroutine::TryExecuteJob() from within the CPU job executed in the
		//      previous step.
		//   3. Thread A begins executing a different CPU job, but this one is also associated with JobCounter
		//      C.
		//   4. An exception is thrown in this job. Thread A decrements the JobCounter C and waits for C to
		//      reach 0. However, because the first job Thread A acquired never had its counter decrement
		//      executed, Thread A waits forever in what is essentially a deadlock.
		//
		// To avoid this, we check if the thread has already "entered" the JobCounter. If so, then we return
		// true here for all entries except the very first one; in that case, we return false unless the
		// JobCounter really has reached 0.
		
		return (mThreadEntryCountArr[Util::Threading::GetThreadLocalResources().GetThreadIndex()] > 1 || !mCounter.load());
	}

	void JobCounter::DecrementCounter()
	{
		--mCounter;
	}

	void JobCounter::SetCounterValue(std::uint32_t jobCount)
	{
		mCounter.store(jobCount);
	}

	void JobCounter::NotifyThreadEntry()
	{
		++(mThreadEntryCountArr[Util::Threading::GetThreadLocalResources().GetThreadIndex()]);
	}

	void JobCounter::NotifyThreadExit()
	{
		--(mThreadEntryCountArr[Util::Threading::GetThreadLocalResources().GetThreadIndex()]);
	}
}