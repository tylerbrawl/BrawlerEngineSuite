module;
#include <optional>
#include <cassert>

module Brawler.ThreadLocalResources;

namespace Brawler
{
	DelayedJobSubmitter& ThreadLocalResources::GetDelayedJobSubmitter()
	{
		return mDelayedJobSubmitter;
	}

	const DelayedJobSubmitter& ThreadLocalResources::GetDelayedJobSubmitter() const
	{
		return mDelayedJobSubmitter;
	}

	void ThreadLocalResources::SetCachedFrameNumber(const std::uint64_t frameNumber)
	{
		mCachedFrameNumberStack.push(frameNumber);
	}

	void ThreadLocalResources::ResetCachedFrameNumber()
	{
		assert(HasCachedFrameNumber() && "ERROR: An attempt was made to reset a thread's cached frame number before it was ever given one!");
		
		mCachedFrameNumberStack.pop();
	}

	bool ThreadLocalResources::HasCachedFrameNumber() const
	{
		return !mCachedFrameNumberStack.empty();
	}

	std::uint64_t ThreadLocalResources::GetCachedFrameNumber() const
	{
		assert(HasCachedFrameNumber() && "ERROR: An attempt was made to get a thread's cached frame number, but it did not have one!");
		return mCachedFrameNumberStack.top();
	}

	void ThreadLocalResources::SetThreadIndex(const std::uint32_t index)
	{
		mThreadIndex = index;
	}

	std::uint32_t ThreadLocalResources::GetThreadIndex() const
	{
		return mThreadIndex;
	}
}