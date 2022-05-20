module;
#include <optional>
#include <cassert>

module Brawler.ThreadLocalResources;

namespace Brawler
{
	void ThreadLocalResources::SetCachedFrameNumber(const std::uint64_t frameNumber)
	{
		if (!mCachedFrameNumber.has_value())
			mCachedFrameNumber = frameNumber;

		++mFrameNumberCacheCount;
	}

	void ThreadLocalResources::ResetCachedFrameNumber()
	{
		assert(mFrameNumberCacheCount >= 0 && "ERROR: An attempt was made to reset a thread's cached frame number before it was ever given one!");
		
		if(--mFrameNumberCacheCount == 0)
			mCachedFrameNumber.reset();
	}

	bool ThreadLocalResources::HasCachedFrameNumber() const
	{
		return mCachedFrameNumber.has_value();
	}

	std::uint64_t ThreadLocalResources::GetCachedFrameNumber() const
	{
		assert(HasCachedFrameNumber() && "ERROR: An attempt was made to get a thread's cached frame number, but it did not have one!");
		return *mCachedFrameNumber;
	}
}