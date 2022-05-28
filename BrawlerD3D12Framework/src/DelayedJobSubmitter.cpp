module;
#include <vector>
#include "DxDef.h"

module Brawler.DelayedJobSubmitter;
import Brawler.WorkerThreadPool;

namespace Brawler
{
	extern WorkerThreadPool& GetWorkerThreadPool();
}

namespace Brawler
{
	void DelayedJobSubmitter::CheckForDelayedJobSubmissions()
	{
		std::erase_if(mSubmissionInfoArr, [] (DelayedJobSubmissionInfo& submissionInfo)
		{
			// If the event has been signalled, then submit the jobs and erase the
			// DelayedJobSubmissionInfo instance.

			if (submissionInfo.HEventPtr->IsEventCompleted())
			{
				for (auto&& job : submissionInfo.DelayedJobArr)
					Brawler::GetWorkerThreadPool().DispatchJob(std::move(job));

				return true;
			}

			return false;
		});
	}

	void DelayedJobSubmitter::AddDelayedJobSubmission(DelayedJobSubmissionInfo&& submissionInfo)
	{
		mSubmissionInfoArr.push_back(std::move(submissionInfo));
	}
}