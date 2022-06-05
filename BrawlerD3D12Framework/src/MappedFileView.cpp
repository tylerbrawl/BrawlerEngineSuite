module;
#include "DxDef.h"

module Brawler.MappedFileView;

namespace
{
	// I initially tried defining this directly in MappedFileView.ixx, but that was causing the
	// value to always be initialized to zero. So, we put it here as a work-around.
	static const std::uint32_t allocationGranularity = [] ()
	{
		SYSTEM_INFO sysInfo{};
		GetSystemInfo(&sysInfo);

		return sysInfo.dwAllocationGranularity;
	}();
}

namespace Brawler
{
	std::uint32_t GetAllocationGranularity()
	{
		return allocationGranularity;
	}
}