module;
#include <array>
#include <iostream>
#include <coroutine>
#include <mutex>
#include "DxDef.h"

module Tests.JobSystem;
import Brawler.Timer;
import Brawler.JobRunner;
import Brawler.JobGroup;

namespace
{
	constexpr const std::size_t NUMBER_ARRAY_SIZE = 20;
	constexpr const std::uint32_t INCREMENTS_PER_ELEMENT = 500000;

	void ResetNumberArray(std::array<std::uint32_t, NUMBER_ARRAY_SIZE>& numArray)
	{
		for (std::size_t i = 0; i < NUMBER_ARRAY_SIZE; ++i)
			numArray[i] = 0;
	}

	void DisplayTestString()
	{
		static std::mutex critSection{};
		static std::uint32_t threadCounter = 0;

		{
			std::scoped_lock<std::mutex> lk{ critSection };

			std::cout << "Thread #" << threadCounter++ << " has arrived." << std::endl;
		}
	}
}

namespace Tests
{
	namespace JobSystem
	{
		Brawler::JobRunner RunJobSystemTests()
		{
			// DISCLAIMER: I am not a benchmarking expert, and I make no guarantees about the performance
			// of these tests.

			// Test 1: A bulk increment of integers is done via the job system and then in a traditional
			// single-threaded manner. (As of writing this, neither increment gets compiled out in Release
			// builds.)
			//
			// By changing the values of NUMBER_ARRAY_SIZE and INCREMENTS_PER_ELEMENT in the anonymous
			// namespace above, you can determine how long a given job for a worker thread should be.

			std::cout << "Beginning job system tests...\n" << std::endl;
			Brawler::Timer t{};

			std::array<std::uint32_t, NUMBER_ARRAY_SIZE> numTestArray{};
			ResetNumberArray(numTestArray);

			t.Start();
			Brawler::JobGroup numTestJobs{};
			numTestJobs.Reserve(numTestArray.size());

			for (std::size_t i = 0; i < numTestArray.size(); ++i)
			{
				std::uint32_t& element = numTestArray[i];
				numTestJobs.AddJob([&element, i] ()
					{
						for (std::uint32_t j = 0; j < INCREMENTS_PER_ELEMENT; ++j)
							++element;
					});
			}

			co_await numTestJobs;
			t.Stop();

			std::cout << "Multi-Threaded Bulk Increment Test Completed - Time Elapsed: " << t.GetElapsedTimeInMilliseconds() << "ms" << std::endl;

			ResetNumberArray(numTestArray);

			Brawler::Timer runTimer{};
			t.Start();

			for (std::size_t i = 0; i < numTestArray.size(); ++i)
			{
				runTimer.Start();
				for (std::size_t j = 0; j < INCREMENTS_PER_ELEMENT; ++j)
					++numTestArray[i];
				runTimer.Stop();

				std::cout << "Loop Duration: " << runTimer.GetElapsedTimeInMicroseconds() << "us" << std::endl;
			}

			t.Stop();

			std::cout << "Single-Threaded Bulk Increment Test Completed - Time Elapsed: " << t.GetElapsedTimeInMilliseconds() << "ms" << std::endl;

			ResetNumberArray(numTestArray);

			// Test 2: A JobGroup consisting of one job is assigned to a worker thread. This
			// thread then spawns its own jobs for other threads to consume. This test is meant to
			// ensure that worker threads are capable of spawning and waiting for jobs.
			//
			// As of writing this, the program compiles and executes as expected. Note that
			// the last line of the lambda function added to singleGroup can be changed to
			// "displayGroup.ExecuteJobs()" in order to avoid having to specify Brawler::JobRunner
			// as its return type.

			Brawler::JobGroup singleGroup{};
			singleGroup.Reserve(1);

			singleGroup.AddJob([] () -> Brawler::JobRunner
			{
				Brawler::JobGroup displayGroup{};
				displayGroup.Reserve(20);

				for (std::size_t i = 0; i < 20; ++i)
					displayGroup.AddJob(DisplayTestString);

				co_await displayGroup;
			});

			co_await singleGroup;

			// Test 3: JobGroup::ExecuteJobsAsync() is tested in the worst way possible: by
			// intentionally allowing for a potential segmentation fault. We will create a
			// string on the stack and copy its pointer into the Job objects. We then
			// allow the jobs to execute asynchronously, making sure that the string is destroyed.
			//
			// As of writing this, the program executes the jobs asynchronously, crashing
			// due to an access violation (as expected). Since it causes the program to crash,
			// the test is commented out by default.
			
			/*
			std::string breakIt{ "You'll NEVER be able to read me!" };
			const char* uglyCStr = breakIt.c_str();
			Brawler::CriticalSection displayCritSection{};  // This will cause an error, too! :)

			Brawler::JobGroup appKiller{};
			appKiller.Reserve(10);

			for (std::size_t i = 0; i < 10; ++i)
				appKiller.AddJob([uglyCStr, &displayCritSection] ()
				{
					std::scoped_lock<Brawler::CriticalSection> lk{ displayCritSection };

					std::cout << "Text: " << uglyCStr << std::endl;
				});

			appKiller.ExecuteJobsAsync();

			// Let the mayhem commence...
			*/
		}
	}
}