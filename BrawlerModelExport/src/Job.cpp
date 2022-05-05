module;
#include <memory>

module Brawler.Job;
import Brawler.JobCounter;
import Brawler.JobPriority;

namespace Brawler
{
	Job::Job() :
		mCallback(),
		mCounterPtr(nullptr),
		mPriority(JobPriority::LOW)
	{}

	Job::Job(std::function<void()>&& callback, std::shared_ptr<JobCounter> counterPtr, JobPriority priority) :
		mCallback(std::move(callback)),
		mCounterPtr(counterPtr),
		mPriority(priority)
	{}

	void Job::Execute()
	{
		mCallback();

		if (mCounterPtr != nullptr)
			mCounterPtr->DecrementCounter();
	}

	JobPriority Job::GetPriority() const
	{
		return mPriority;
	}
}