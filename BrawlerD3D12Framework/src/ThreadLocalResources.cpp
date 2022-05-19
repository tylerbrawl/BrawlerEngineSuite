module;
#include <optional>
#include <cassert>

module Brawler.ThreadLocalResources;

namespace Brawler
{
	void ThreadLocalResources::SetCachedFrameNumber(const std::uint64_t frameNumber)
	{
		mCachedFrameNumber = frameNumber;
	}

	void ThreadLocalResources::ResetCachedFrameNumber()
	{
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