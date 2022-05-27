module;
#include <vector>

export module Brawler.DelayedJobSubmitter;
import Brawler.Job;
import Brawler.Win32.SafeHandle;

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
			Win32::SafeHandle HEvent;
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