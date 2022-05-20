module;
#include <memory>
#include <cassert>

module Brawler.Job;
import Brawler.JobCounter;
import Brawler.JobPriority;
import Util.Engine;
import Util.Threading;
import Brawler.ThreadLocalResources;

namespace Brawler
{
	Job::Job(std::function<void()>&& callback, std::shared_ptr<JobCounter> counterPtr, JobPriority priority) :
		mCallback(std::move(callback)),
		mCounterPtr(counterPtr),
		mPriority(priority),

		// Yes, we want to store it on CPU job *creation*, and *NOT* on CPU job execution. If
		// we wait until Job::Execute() to set mCachedFrameNumber, then the value which is
		// cached may end up being different from what the API user expects. This is important
		// if, e.g., a job is left in the queue for an extended period of time.
		//
		// Besides, if the user really wants the actual frame number when the job gets executed,
		// then they can just call Util::Engine::GetTrueFrameNumber() themselves.
		mCachedFrameNumber(Util::Engine::GetCurrentFrameNumber())
	{}

	void Job::Execute()
	{
		ThreadLocalResources& threadLocalResources{ Util::Threading::GetThreadLocalResources() };
		threadLocalResources.SetCachedFrameNumber(mCachedFrameNumber);
		
		try
		{
			mCallback();

			threadLocalResources.ResetCachedFrameNumber();

			if (mCounterPtr != nullptr)
				mCounterPtr->DecrementCounter();
		}
		catch (...)
		{
			threadLocalResources.ResetCachedFrameNumber();
			
			if (mCounterPtr != nullptr)
				mCounterPtr->DecrementCounter();

			std::rethrow_exception(std::current_exception());
		}
	}

	JobPriority Job::GetPriority() const
	{
		return mPriority;
	}
}