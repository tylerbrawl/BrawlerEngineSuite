module;
#include <memory>
#include <optional>
#include <exception>

module Brawler.Job;
import Brawler.JobCounter;
import Brawler.JobPriority;
import Util.Coroutine;
import Brawler.JobCounterGuard;
import Util.Win32;

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
		catch (const std::exception& e)
		{
			Util::Win32::WriteFormattedConsoleMessage(e.what(), Util::Win32::ConsoleFormat::CRITICAL_FAILURE);
			std::terminate();
		}
		catch (...)
		{
			Util::Win32::WriteFormattedConsoleMessage(L"ERROR: An unrecoverable error was detected. The program has been terminated.", Util::Win32::ConsoleFormat::CRITICAL_FAILURE);
			std::terminate();
		}
	}

	JobPriority Job::GetPriority() const
	{
		return mPriority;
	}
}