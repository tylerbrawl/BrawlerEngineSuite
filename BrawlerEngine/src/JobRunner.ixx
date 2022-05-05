module;
#include <coroutine>
#include <memory>

export module Brawler.JobRunner;
import Brawler.JobCounter;

export namespace Brawler
{
	// This struct is used to handle synchronous job execution. It is primarily used as a means
	// to co_await the completion of all of the jobs within a JobGroup.

	class JobGroup;

	struct JobRunner
	{
		struct Awaiter
		{
			std::shared_ptr<JobCounter> Counter;

			bool await_ready() const;
			std::coroutine_handle<> await_suspend(std::coroutine_handle<> hCoroutine);
			void await_resume() const;
		};

		struct promise_type
		{
			JobRunner get_return_object();

			// This await_transform() allows one to use "co_await [JobGroup Instance]" to
			// synchronously execute jobs.
			Awaiter await_transform(JobGroup& rhs) const;

			// We will wait to suspend until co_await is used in a coroutine.
			std::suspend_never initial_suspend() const;

			std::suspend_never final_suspend() noexcept;
			void return_void() const;

			void unhandled_exception() const;
		};
	};
}