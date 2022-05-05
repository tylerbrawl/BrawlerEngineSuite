module;
#include <cstdint>

export module Brawler.Timer;

export namespace Brawler
{
	class Timer
	{
	private:
		enum class State
		{
			RUNNING,
			PAUSED,
			STOPPED
		};

	public:
		Timer();

		// Starts (or restarts, if it was already running) the timer. 
		// By default, the timer is stopped.
		void Start();

		// Pauses the timer. 
		// Calling Timer::Start() will resume the timer without affecting the elapsed time total.
		void Pause();

		// Stops the timer. This action is irreversible.
		// The elapsed time total is only reset upon calling Timer::Start().
		void Stop();

		bool IsPaused() const;
		bool IsStopped() const;

		// Returns the elapsed time in seconds.
		float GetElapsedTimeInSeconds();

		// Returns the elapsed time in milliseconds.
		float GetElapsedTimeInMilliseconds();

		// Returns the elapsed time in microseconds.
		float GetElapsedTimeInMicroseconds();

	private:
		std::int64_t GetCurrentTimeInTicks() const;
		std::int64_t GetElapsedTimeInTicks();
		std::int64_t GetPeriod() const;

	private:
		std::int64_t mStartTime;
		std::int64_t mCurrentTime;
		std::int64_t mTicksPaused;
		State mState;
	};
}