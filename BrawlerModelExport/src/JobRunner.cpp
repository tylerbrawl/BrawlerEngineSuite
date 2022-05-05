module;
#include <stdexcept>
#include <memory>
#include <optional>
#include <thread>

module Brawler.JobRunner;
import Brawler.Application;
import Brawler.WorkerThreadPool;
import Brawler.JobGroup;
import Brawler.JobCounter;
import Brawler.Job;
import Util.Coroutine;
import Util.Threading;

namespace Brawler
{
	bool JobRunner::Awaiter::await_ready() const
	{
		return (Counter != nullptr ? Counter->IsFinished() : true);
	}

	std::coroutine_handle<> JobRunner::Awaiter::await_suspend(std::coroutine_handle<> hCoroutine)
	{
		// At this point, the current thread will be waiting for the execution of all of the
		// jobs within a JobGroup. We will not be transferring this std::coroutine_handle to
		// a different thread, so we can safely access the Counter member of *this.

		// A naive implementation would simply wait for the relevant jobs to finish execution,
		// but we can do better than that. Instead, we will try to steal jobs from worker
		// threads.

		while (!Counter->IsFinished())
			Util::Coroutine::TryExecuteJob();

		// When we are finished, resume the coroutine, since the co_await may not be the
		// last statement in it.
		return hCoroutine;
	}

	void JobRunner::Awaiter::await_resume() const
	{}

	JobRunner JobRunner::promise_type::get_return_object()
	{
		return JobRunner{};
	}

	JobRunner::Awaiter JobRunner::promise_type::await_transform(JobGroup& rhs) const
	{
		// We need to dispatch the Jobs from the JobGroup here, so that calling co_await
		// on a JobGroup instance will still send jobs to the queues.
		rhs.DispatchJobs();
		
		return Awaiter{ rhs.mCounter };
	}

	std::suspend_never JobRunner::promise_type::initial_suspend() const
	{
		return std::suspend_never{};
	}

	std::suspend_never JobRunner::promise_type::final_suspend() noexcept
	{
		return std::suspend_never{};
	}

	void JobRunner::promise_type::return_void() const
	{}

	void JobRunner::promise_type::unhandled_exception() const
	{
		Util::Coroutine::RethrowUnhandledException();
	}
}