module;

module Brawler.JobCounterGuard;

namespace Brawler
{
	JobCounterGuard::JobCounterGuard(JobCounter* counterPtr) :
		mCounterPtr(counterPtr)
	{
		if (mCounterPtr != nullptr) [[likely]]
			mCounterPtr->NotifyThreadEntry();
	}

	JobCounterGuard::~JobCounterGuard()
	{
		if (mCounterPtr != nullptr) [[likely]]
			mCounterPtr->NotifyThreadExit();
	}
}