module;
#include <cassert>
#include "DxDef.h"

module Brawler.Win32EventHandle;

namespace Brawler
{
	Win32EventHandle::Win32EventHandle(Win32::SafeHandle&& hEvent) :
		I_EventHandle(),
		mHEvent(std::move(hEvent))
	{
		assert(mHEvent != nullptr && "ERROR: Win32EventHandle was never given a valid event handle type!");
	}
	
	bool Win32EventHandle::IsEventCompleted() const
	{
		return (WaitForSingleObject(mHEvent.get(), 0) == WAIT_OBJECT_0);
	}
}