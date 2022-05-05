module;
#include <cstdint>
#include <memory>
#include <coroutine>
#include <functional>

module Brawler.JobGroup;
import Brawler.Application;
import Brawler.JobPriority;
import Brawler.JobCounter;

namespace Brawler
{
	JobGroup::JobGroup(JobPriority priority, std::size_t initialReservedCount) :
		mCounter(std::make_shared<JobCounter>()),
		mJobArr(),
		mPriority(priority)
	{
		mJobArr.reserve(initialReservedCount);
	}

	void JobGroup::AddJob(std::function<void()>&& job)
	{
		mJobArr.push_back(Job{ std::move(job), mCounter, mPriority });
	}

	void JobGroup::Reserve(std::size_t jobCount)
	{
		mJobArr.reserve(jobCount);
	}

	JobRunner JobGroup::ExecuteJobs()
	{
		// JobRunner::promise_type::await_transform() calls JobGroup::DispatchJobs() for
		// us, so we should not do it here. (We do it like this so that calling co_await
		// on a JobGroup instance will work.)
		
		co_await *this;
	}

	void JobGroup::ExecuteJobsAsync()
	{
		// Synchronous execution only happens when co_await is used. Thus, we can simply
		// send the jobs to the queues without worry.

		for (auto&& job : mJobArr)
			Brawler::GetApplication().GetWorkerThreadPool().DispatchJob(std::move(job));

		mJobArr.clear();
	}

	void JobGroup::DispatchJobs()
	{
		// Since this is for synchronous execution of jobs, we need to make sure that
		// the counter is up-to-date *before* adding the jobs to the worker thread queues.

		mCounter->SetCounterValue(static_cast<std::uint32_t>(mJobArr.size()));

		for (auto&& job : mJobArr)
			Brawler::GetApplication().GetWorkerThreadPool().DispatchJob(std::move(job));

		mJobArr.clear();
	}
}