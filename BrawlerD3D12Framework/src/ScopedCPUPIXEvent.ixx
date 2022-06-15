module;
#include <compare>

export module Brawler.D3D12.ScopedCPUPIXEvent;
import :DebugScopedCPUPIXEvent;
import Brawler.NZStringView;
import Util.General;

namespace Brawler
{
	namespace D3D12
	{
		class ReleaseScopedCPUPIXEvent
		{
		public:
			ReleaseScopedCPUPIXEvent(const Brawler::NZStringView eventName)
			{}

			ReleaseScopedCPUPIXEvent(const Brawler::NZWStringView eventName)
			{}

			// Even if this class does nothing, we still want to delete the copy functions,
			// since they are deleted in the Debug version. Otherwise, we might have code which
			// compiles in Release builds, but not in Debug builds.
			
			ReleaseScopedCPUPIXEvent(const ReleaseScopedCPUPIXEvent& rhs) = delete;
			ReleaseScopedCPUPIXEvent& operator=(const ReleaseScopedCPUPIXEvent& rhs) = delete;

			ReleaseScopedCPUPIXEvent(ReleaseScopedCPUPIXEvent&& rhs) noexcept = default;
			ReleaseScopedCPUPIXEvent& operator=(ReleaseScopedCPUPIXEvent&& rhs) noexcept = default;
		};
	}
}

namespace Brawler
{
	namespace D3D12
	{
		template <Util::General::BuildMode BuildMode>
		struct EventTypeSolver
		{
			using EventType = DebugScopedCPUPIXEvent;
		};

		template <>
		struct EventTypeSolver<Util::General::BuildMode::RELEASE>
		{
			using EventType = ReleaseScopedCPUPIXEvent;
		};
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		using ScopedCPUPIXEvent = typename EventTypeSolver<Util::General::GetBuildMode()>::EventType;
	}
}