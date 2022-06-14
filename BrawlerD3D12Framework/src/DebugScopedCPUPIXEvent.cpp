module;
#include "DxDef.h"

module Brawler.D3D12.ScopedCPUPIXEvent;
import Util.D3D12;
import Brawler.NZStringView;

// Define our own versions of the PIX functions here if we are not #including pix3.h.
#if !defined(_DEBUG) && !defined(__RELEASE_WITH_DEBUGGING__)
namespace
{
	template <typename... Args>
	__forceinline void PIXBeginEvent(Args&&... args)
	{}

	consteval void PIXEndEvent()
	{}
}
#endif

namespace Brawler
{
	namespace D3D12
	{
		DebugScopedCPUPIXEvent::DebugScopedCPUPIXEvent(const Brawler::NZStringView eventName) :
			mOwnsCurrentEvent(true)
		{
			if constexpr (Util::D3D12::IsPIXRuntimeSupportEnabled())
				PIXBeginEvent(Util::D3D12::PIX_EVENT_COLOR_CPU_ONLY, eventName.C_Str());
		}

		DebugScopedCPUPIXEvent::DebugScopedCPUPIXEvent(const Brawler::NZWStringView eventName) :
			mOwnsCurrentEvent(true)
		{
			if constexpr (Util::D3D12::IsPIXRuntimeSupportEnabled())
				PIXBeginEvent(Util::D3D12::PIX_EVENT_COLOR_CPU_ONLY, eventName.C_Str());
		}

		DebugScopedCPUPIXEvent::DebugScopedCPUPIXEvent(DebugScopedCPUPIXEvent&& rhs) noexcept :
			mOwnsCurrentEvent(rhs.mOwnsCurrentEvent)
		{
			rhs.mOwnsCurrentEvent = false;
		}

		DebugScopedCPUPIXEvent& DebugScopedCPUPIXEvent::operator=(DebugScopedCPUPIXEvent&& rhs) noexcept
		{
			EndEvent();

			mOwnsCurrentEvent = rhs.mOwnsCurrentEvent;
			rhs.mOwnsCurrentEvent = false;

			return *this;
		}

		DebugScopedCPUPIXEvent::~DebugScopedCPUPIXEvent()
		{
			EndEvent();
		}

		void DebugScopedCPUPIXEvent::EndEvent()
		{
			if constexpr (Util::D3D12::IsPIXRuntimeSupportEnabled())
			{
				if (mOwnsCurrentEvent)
				{
					PIXEndEvent();
					mOwnsCurrentEvent = false;
				}
			}
		}
	}
}