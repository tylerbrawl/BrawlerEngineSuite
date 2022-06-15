module;
#include "DxDef.h"

export module Brawler.D3D12.ScopedCPUPIXEvent:DebugScopedCPUPIXEvent;
import Brawler.NZStringView;

export namespace Brawler
{
	namespace D3D12
	{
		class DebugScopedCPUPIXEvent
		{
		public:
			explicit DebugScopedCPUPIXEvent(const Brawler::NZStringView eventName);
			explicit DebugScopedCPUPIXEvent(const Brawler::NZWStringView eventName);

			~DebugScopedCPUPIXEvent();

			DebugScopedCPUPIXEvent(const DebugScopedCPUPIXEvent& rhs) = delete;
			DebugScopedCPUPIXEvent& operator=(const DebugScopedCPUPIXEvent& rhs) = delete;

			DebugScopedCPUPIXEvent(DebugScopedCPUPIXEvent&& rhs) noexcept;
			DebugScopedCPUPIXEvent& operator=(DebugScopedCPUPIXEvent&& rhs) noexcept;

		private:
			void EndEvent();

		private:
			bool mOwnsCurrentEvent;
		};
	}
}