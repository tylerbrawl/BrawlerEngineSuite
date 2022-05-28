module;

module Brawler.I_EventHandle;
import Util.Coroutine;

namespace Brawler
{
	void I_EventHandle::WaitForEventCompletion(const bool executeJobsWhileWaiting) const
	{
		if (executeJobsWhileWaiting)
		{
			while (!IsEventCompleted())
				Util::Coroutine::TryExecuteJob();
		}
		else
			while (!IsEventCompleted());
	}
}