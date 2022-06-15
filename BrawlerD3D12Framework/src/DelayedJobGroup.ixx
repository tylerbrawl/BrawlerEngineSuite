module;
#include <vector>
#include <functional>
#include <concepts>
#include <memory>

export module Brawler.DelayedJobGroup;
import Brawler.Job;
import Brawler.JobPriority;
import Brawler.I_EventHandle;
import Brawler.ThreadLocalResources;
import Util.Threading;
import Brawler.DelayedJobSubmitter;
import Brawler.Functional;
import Brawler.CustomEventHandle;

namespace Brawler
{
	template <typename Callback>
	concept IsCustomEventCompletionCallback = requires (Callback&& callback)
	{
		CustomEventHandle<Callback>{ std::move(callback) };

		// bool Callback::operator()
		{ callback() } -> std::same_as<bool>;
	};
}

export namespace Brawler
{
	class DelayedJobGroup
	{
	public:
		explicit DelayedJobGroup(const JobPriority priority = JobPriority::NORMAL);

		DelayedJobGroup(const DelayedJobGroup& rhs) = delete;
		DelayedJobGroup& operator=(const DelayedJobGroup& rhs) = delete;

		DelayedJobGroup(DelayedJobGroup&& rhs) noexcept = default;
		DelayedJobGroup& operator=(DelayedJobGroup&& rhs) noexcept = default;

		void AddJob(std::move_only_function<void()>&& callback);
		void Reserve(const std::size_t jobCount);

		template <typename T>
			requires std::derived_from<T, I_EventHandle>
		void SubmitDelayedJobs(T&& hEvent);

		/// <summary>
		/// Queues the current list of CPU jobs to the calling thread's queue of delayed jobs. The
		/// CPU jobs will only be added to the queues in the WorkerThreadPool when the callback
		/// function specified by completionCallback returns true.
		/// 
		/// Prior to executing any CPU job, a thread checks its queue of delayed jobs to see if any
		/// of the events corresponding to said jobs have been signalled. If so, they are immediately
		/// dispatched to the WorkerThreadPool.
		/// </summary>
		/// <typeparam name="Callback">
		/// - The (closure/anonymous) type of completionCallback. This can be deduced as that of a
		///   lambda function. The type must fulfill the Brawler::Function&lt;bool&gt; concept.
		/// </typeparam>
		/// <param name="completionCallback">
		/// - The callback which will be called occasionally to see if the CPU jobs added to this
		///   DelayedJobGroup instance can be submitted. The callback should return true if the jobs
		///   can be immediately submitted to the WorkerThreadPool's queue and false otherwise.
		/// </param>
		template <typename Callback>
			requires IsCustomEventCompletionCallback<Callback>
		void SubmitDelayedJobs(Callback&& completionCallback);

	private:
		std::vector<Job> mJobArr;
		JobPriority mPriority;
	};
}

// ----------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename T>
		requires std::derived_from<T, I_EventHandle>
	void DelayedJobGroup::SubmitDelayedJobs(T&& hEvent)
	{
		Util::Threading::GetThreadLocalResources().GetDelayedJobSubmitter().AddDelayedJobSubmission(DelayedJobSubmitter::DelayedJobSubmissionInfo{
			.DelayedJobArr{ std::move(mJobArr) },
			.HEventPtr{ std::make_unique<T>(std::move(hEvent)) }
		});
	}

	template <typename Callback>
		requires IsCustomEventCompletionCallback<Callback>
	void DelayedJobGroup::SubmitDelayedJobs(Callback&& completionCallback)
	{
		Util::Threading::GetThreadLocalResources().GetDelayedJobSubmitter().AddDelayedJobSubmission(DelayedJobSubmitter::DelayedJobSubmissionInfo{
			.DelayedJobArr{ std::move(mJobArr) },
			.HEventPtr{ std::make_unique<CustomEventHandle<Callback>>(std::move(completionCallback)) }
		});
	}
}