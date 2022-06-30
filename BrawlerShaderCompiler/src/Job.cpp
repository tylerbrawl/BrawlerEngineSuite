module;
#include <memory>
#include <optional>

module Brawler.Job;
import Brawler.JobCounter;
import Brawler.JobPriority;
import Util.Coroutine;
import Brawler.JobCounterGuard;

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
		JobCounterGuard counterGuard{ mCounterPtr.get() };
		
		try
		{
			mCallback();

			if (mCounterPtr != nullptr)
				mCounterPtr->DecrementCounter();
		}
		catch (...)
		{
			if (mCounterPtr != nullptr)
			{
				mCounterPtr->DecrementCounter();
				
				while (!mCounterPtr->IsFinished())
					Util::Coroutine::TryExecuteJob();
			}
			
			std::rethrow_exception(std::current_exception());
		}
	}

	JobPriority Job::GetPriority() const
	{
		return mPriority;
	}
}