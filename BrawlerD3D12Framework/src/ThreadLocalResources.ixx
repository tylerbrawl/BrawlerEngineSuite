module;
#include <stack>
#include <vector>

export module Brawler.ThreadLocalResources;
import Brawler.DelayedJobSubmitter;

export namespace Brawler
{
	// Add data which each WorkerThread should keep its own version of to this class.

	class ThreadLocalResources
	{
	public:
		ThreadLocalResources() = default;

		ThreadLocalResources(const ThreadLocalResources& rhs) = delete;
		ThreadLocalResources& operator=(const ThreadLocalResources& rhs) = delete;

		ThreadLocalResources(ThreadLocalResources&& rhs) noexcept = default;
		ThreadLocalResources& operator=(ThreadLocalResources&& rhs) noexcept = default;

		DelayedJobSubmitter& GetDelayedJobSubmitter();
		const DelayedJobSubmitter& GetDelayedJobSubmitter() const;

		void SetCachedFrameNumber(const std::uint64_t frameNumber);
		void ResetCachedFrameNumber();

		bool HasCachedFrameNumber() const;
		std::uint64_t GetCachedFrameNumber() const;

		void SetThreadIndex(const std::uint32_t index);
		std::uint32_t GetThreadIndex() const;

	private:
		DelayedJobSubmitter mDelayedJobSubmitter;
		std::stack<std::uint64_t> mCachedFrameNumberStack;
		std::uint32_t mThreadIndex;
	};
}