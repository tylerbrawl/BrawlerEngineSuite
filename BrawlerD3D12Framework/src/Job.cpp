module;
#include <memory>
#include <cassert>
#include <functional>

module Brawler.Job;
import Brawler.JobCounter;
import Brawler.JobPriority;
import Util.Engine;
import Util.Threading;
import Brawler.ThreadLocalResources;
import Brawler.DelayedJobSubmitter;
import Brawler.JobCounterGuard;

namespace Brawler
{
	Job::Job(std::move_only_function<void()>&& callback, std::shared_ptr<JobCounter> counterPtr, JobPriority priority) :
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
		JobCounterGuard counterGuard{ mCounterPtr.get() };
		
		ThreadLocalResources& threadLocalResources{ Util::Threading::GetThreadLocalResources() };
		threadLocalResources.SetCachedFrameNumber(mCachedFrameNumber);

		// Try to submit any delayed CPU jobs belonging to this thread.
		Util::Threading::GetThreadLocalResources().GetDelayedJobSubmitter().CheckForDelayedJobSubmissions();
		
		try
		{
			mCallback();

			if (mCounterPtr != nullptr)
				mCounterPtr->DecrementCounter();

			threadLocalResources.ResetCachedFrameNumber();
		}
		catch (...)
		{
			threadLocalResources.ResetCachedFrameNumber();
			
			if (mCounterPtr != nullptr)
			{
				mCounterPtr->DecrementCounter();

				// We don't want this thread to exit until all of the other CPU jobs associated
				// with this counter have completed; otherwise, we risk stack unwinding wreaking
				// havoc on the memory accessed by other threads. So, we wait here until the
				// counter reaches zero to leave.
				while (!mCounterPtr->IsFinished());
			}

			std::rethrow_exception(std::current_exception());
		}
	}

	JobPriority Job::GetPriority() const
	{
		return mPriority;
	}
}