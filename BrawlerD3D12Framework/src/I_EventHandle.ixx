module;

export module Brawler.I_EventHandle;

export namespace Brawler
{
	class I_EventHandle
	{
	protected:
		I_EventHandle() = default;

	public:
		virtual ~I_EventHandle() = default;

		I_EventHandle(const I_EventHandle& rhs) = default;
		I_EventHandle& operator=(const I_EventHandle& rhs) = default;

		I_EventHandle(I_EventHandle&& rhs) noexcept = default;
		I_EventHandle& operator=(I_EventHandle&& rhs) noexcept = default;

		virtual bool IsEventCompleted() const = 0;

		/// <summary>
		/// Blocks the calling thread until this I_EventHandle instance is marked
		/// as completed (i.e., IsEventCompleted() returns true). If executeJobsWhileWaiting
		/// is set to true, then Util::Coroutine::TryExecuteJob() is called while the
		/// I_EventHandle instance specifies that it is not completed. If no value is
		/// specified for executeJobsWhileWaiting, then it is set to true by default.
		/// 
		/// Most of the time, you will want to leave executeJobsWhileWaiting as true.
		/// However, for latency-critical tasks, you may want to consider setting this
		/// to false.
		/// </summary>
		/// <param name="executeJobsWhileWaiting">
		/// - Describes whether or not Util::Coroutine::TryExecuteJob() is called while
		///   waiting for event completion. For latency-critical tasks, you may want to
		///   set this to false; otherwise, this should be set to true in order to maximize
		///   throughput. By default, this parameter is set to true.
		/// </param>
		void WaitForEventCompletion(const bool executeJobsWhileWaiting = true) const;
	};
}