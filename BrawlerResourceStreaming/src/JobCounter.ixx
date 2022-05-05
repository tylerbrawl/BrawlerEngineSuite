module;
#include <atomic>

export module Brawler.JobCounter;

export namespace Brawler
{
	class JobGroup;
}

export namespace Brawler
{
	class JobCounter
	{
	private:
		friend class JobGroup;

	public:
		JobCounter();

		void DecrementCounter();
		bool IsFinished() const;

	private:
		void SetCounterValue(std::uint32_t jobCount);

	private:
		std::atomic<std::uint32_t> mCounter;
	};
}