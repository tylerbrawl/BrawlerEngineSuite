module;

module Brawler.RenderEventHandle;
import Util.Coroutine;

namespace Brawler
{
	RenderEventHandle::RenderEventHandle(const std::shared_ptr<std::atomic<bool>> flag) :
		mFlag(flag)
	{}

	bool RenderEventHandle::IsEventCompleted() const
	{
		return mFlag->load();
	}

	void RenderEventHandle::WaitForEventCompletion() const
	{
		while (!(mFlag->load()))
			Util::Coroutine::TryExecuteJob();
	}
}