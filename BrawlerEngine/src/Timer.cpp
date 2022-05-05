module;
#include <chrono>

module Brawler.Timer;

namespace Brawler
{
	Timer::Timer() :
		mStartTime(0),
		mCurrentTime(0),
		mTicksPaused(0),
		mState(Timer::State::STOPPED)
	{}

	void Timer::Start()
	{
		if (IsStopped())
			mStartTime = GetCurrentTimeInTicks();

		if (IsPaused())
			mTicksPaused += GetCurrentTimeInTicks() - mCurrentTime;
		else
			mTicksPaused = 0;

		mState = Timer::State::RUNNING;
	}

	void Timer::Pause()
	{
		if (IsPaused() || IsStopped())
			return;

		mCurrentTime = GetCurrentTimeInTicks();
		mState = Timer::State::PAUSED;
	}

	void Timer::Stop()
	{
		if (IsStopped())
			return;

		mCurrentTime = GetCurrentTimeInTicks();
		mState = Timer::State::STOPPED;
	}

	bool Timer::IsPaused() const
	{
		return (mState == Timer::State::PAUSED);
	}

	bool Timer::IsStopped() const
	{
		return (mState == Timer::State::STOPPED);
	}

	float Timer::GetElapsedTimeInSeconds()
	{
		return (static_cast<float>(GetElapsedTimeInTicks()) / GetPeriod());
	}

	float Timer::GetElapsedTimeInMilliseconds()
	{
		// From the MSDN:
		// To guard against loss-of-precision, we convert
		// to milliseconds *before* dividing by ticks-per-second.

		std::int64_t ticksMS = GetElapsedTimeInTicks() * static_cast<std::int64_t>(1000);
		return (ticksMS / static_cast<float>(GetPeriod()));
	}

	float Timer::GetElapsedTimeInMicroseconds()
	{
		// From the MSDN:
		// To guard against loss-of-precision, we convert
		// to microseconds *before* dividing by ticks-per-second.

		std::int64_t ticksUS = GetElapsedTimeInTicks() * static_cast<std::int64_t>(1000000);
		return (ticksUS / static_cast<float>(GetPeriod()));
	}

	std::int64_t Timer::GetCurrentTimeInTicks() const
	{
		return std::chrono::high_resolution_clock::now().time_since_epoch().count();
	}

	std::int64_t Timer::GetElapsedTimeInTicks()
	{
		std::uint64_t currentTicks = GetCurrentTimeInTicks();

		if (IsPaused())
			mTicksPaused += currentTicks - mCurrentTime;
		
		if (!IsStopped())
			mCurrentTime = currentTicks;


		return (mCurrentTime - mStartTime - mTicksPaused);
	}

	std::int64_t Timer::GetPeriod() const
	{
		return std::chrono::high_resolution_clock::period::den;
	}
}