module;
#include <vector>
#include <functional>
#include <coroutine>

export module Brawler.JobGroup;
import Brawler.Job;
import Brawler.JobRunner;
import Brawler.JobPriority;
import Brawler.JobCounter;

export namespace Brawler
{
	class JobGroup
	{
	private:
		friend struct JobRunner;

	public:
		JobGroup(JobPriority priority = JobPriority::NORMAL, std::size_t initialReservedCount = 1);

		// Adds a job to the JobGroup. The job will not be executed until either
		// JobGroup::ExecuteJobs() or JobGroup::ExecuteJobsAsync() is called.
		void AddJob(std::move_only_function<void()>&& job);

		// Allocates memory for the specified number of jobs.
		void Reserve(std::size_t jobCount);

		// Executes the jobs added to the JobGroup synchronously. The return from this
		// call synchronizes-with (i.e., happens after) all jobs have been executed.
		//
		// NOTE: This is equivalent to calling "co_await [JobGroup Instance]."
		JobRunner ExecuteJobs();

		// Executes the jobs added to the JobGroup asynchronously. This is useful for
		// long-running tasks with no dependent jobs, such as file I/O.
		void ExecuteJobsAsync();

	private:
		void DispatchJobs();

	private:
		std::shared_ptr<JobCounter> mCounter;
		std::vector<Job> mJobArr;
		const JobPriority mPriority;
	};
}