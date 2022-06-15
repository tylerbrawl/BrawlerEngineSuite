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

		/// <summary>
		/// Describes whether or not this DelayedJobGroup instance has any delayed CPU jobs
		/// which must be checked. 
		/// 
		/// This does *NOT* indicate that any contained delayed CPU jobs are actually ready 
		/// for submission; it only means that there are some which may be ready for submission 
		/// eventually.
		/// </summary>
		/// <returns>
		/// The function returns true if this DelayedJobGroup instance has any delayed CPU jobs
		/// which must be checked and false otherwise.
		/// </returns>
		bool HasDelayedJobsToCheck() const;

	private:
		void AddDelayedJobSubmission(DelayedJobSubmissionInfo&& submissionInfo);

	private:
		std::vector<DelayedJobSubmissionInfo> mSubmissionInfoArr;
	};
}