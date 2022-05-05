module;

export module Brawler.JobPriority;

export namespace Brawler
{
	enum class JobPriority
	{
		LOW,
		NORMAL,
		HIGH,
		CRITICAL,

		COUNT
	};
}