module;
#include <memory>
#include <vector>

export module Brawler.DelayedJobSubmitter;
import Brawler.Job;
import Brawler.I_EventHandle;

export namespace Brawler
{
	class DelayedJobGroup;
}

export namespace Brawler
{
	class DelayedJobSubmitter
	{
	private:
		struct DelayedJobSubmissionInfo
		{
			std::vector<Job> DelayedJobArr;
			std::unique_ptr<I_EventHandle> HEventPtr;
		};

	private:
		friend class DelayedJobGroup;

	public:
		DelayedJobSubmitter() = default;

		DelayedJobSubmitter(const DelayedJobSubmitter& rhs) = delete;
		DelayedJobSubmitter& operator=(const DelayedJobSubmitter& rhs) = delete;

		DelayedJobSubmitter(DelayedJobSubmitter&& rhs) noexcept = default;
		DelayedJobSubmitter& operator=(DelayedJobSubmitter&& rhs) noexcept = default;

		void CheckForDelayedJobSubmissions();

	private:
		void AddDelayedJobSubmission(DelayedJobSubmissionInfo&& submissionInfo);

	private:
		std::vector<DelayedJobSubmissionInfo> mSubmissionInfoArr;
	};
}