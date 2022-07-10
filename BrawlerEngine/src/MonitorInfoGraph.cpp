module;
#include <vector>
#include <unordered_map>
#include <mutex>
#include <stdexcept>
#include <cassert>
#include <ranges>
#include <DxDef.h>

module Brawler.MonitorInfoGraph;
import Util.General;
import Util.Engine;

namespace Brawler
{
	void MonitorInfoGraph::RegisterMonitor(const Monitor& monitor)
	{
		std::scoped_lock<std::mutex> lock{ mCritSection };

		AssignMonitorToMap(monitor);
	}

	void MonitorInfoGraph::UpdateCachedMonitorInformation()
	{
		std::vector<DISPLAYCONFIG_PATH_INFO> pathInfoArr{};
		std::vector<DISPLAYCONFIG_MODE_INFO> modeInfoArr{};

		while (true)
		{
			static constexpr std::uint32_t DISPLAY_CONFIG_FLAGS = (QDC_ONLY_ACTIVE_PATHS | QDC_VIRTUAL_MODE_AWARE | QDC_INCLUDE_HMD);

			std::uint32_t pathInfoCount = 0;
			std::uint32_t modeInfoCount = 0;

			if (GetDisplayConfigBufferSizes(DISPLAY_CONFIG_FLAGS, &pathInfoCount, &modeInfoCount) != ERROR_SUCCESS) [[unlikely]]
				throw std::runtime_error{ "ERROR: An attempt to get information regarding the connected monitors failed!" };

			pathInfoArr.resize(pathInfoCount);
			modeInfoArr.resize(modeInfoCount);

			const std::int32_t queryDisplayConfigResult = QueryDisplayConfig(
				DISPLAY_CONFIG_FLAGS,
				&pathInfoCount,
				pathInfoArr.data(),
				&modeInfoCount,
				modeInfoArr.data(),
				nullptr
			);

			if (queryDisplayConfigResult == ERROR_SUCCESS) [[likely]]
			{
				pathInfoArr.resize(pathInfoCount);
				modeInfoArr.resize(modeInfoCount);

				break;
			}
			else if (queryDisplayConfigResult != ERROR_INSUFFICIENT_BUFFER) [[unlikely]]
				throw std::runtime_error{ "ERROR: An attempt to get information regarding the connected monitors failed!" };
		}

		// The QueryDisplayConfig() API is rather confusing, in my opinion, so I'll go ahead and explain it
		// here. Basically, after a successful call to QueryDisplayConfig(), pathInfoArr will contain one
		// display configuration entry for each active display output. (We limited it to only active display
		// outputs by using the QDC_ONLY_ACTIVE_PATHS flag.) Each display configuration entry consists of
		// three components:
		//
		//   - Source: The source is the surface onto which the display adapter can render pixels. It is
		//     described by its area rectangle in desktop coordinates.
		//
		//   - Target: The target is one of many possible video outputs for a display adapter. If the source
		//     is the logical surface which the adapter renders to, then the target is the physical monitor
		//     which displays this source. There are likely to be far more targets than there are actual
		//     connectors on the display adapter, since each connector has multiple targets for the sake of
		//     backwards compatibility. For instance, a DVI connector exposes both a DVI target and a VGA
		//     target.
		//
		//   - Desktop Image Info: This describes the parts of the source which are actually being written
		//     to. It is similar in concept to the idea of a scissor rectangle from the D3D12 API.
		//
		// What we are going to do is combine all of this information into a single Win32::MonitorInformation
		// structure. In doing so, it becomes much easier to reason about what data is relevant for which monitor.

		const LUID relevantAdapterLUID{ Util::Engine::GetD3D12Device().GetAdapterLuid() };
		const auto filterPathInfoLambda = [relevantAdapterLUID] (const DISPLAYCONFIG_PATH_INFO& pathInfo)
		{
			// Filter out inactive paths.
			if ((pathInfo.flags & DISPLAYCONFIG_PATH_ACTIVE) == 0) [[unlikely]]
				return false;
			
			// Only worry about monitors which are connected to the D3D12 device we are working with.
			if (pathInfo.sourceInfo.adapterId.HighPart != relevantAdapterLUID.HighPart || pathInfo.sourceInfo.adapterId.LowPart != relevantAdapterLUID.LowPart) [[unlikely]]
				return false;

			return (pathInfo.targetInfo.adapterId.HighPart == relevantAdapterLUID.HighPart && pathInfo.targetInfo.adapterId.LowPart == relevantAdapterLUID.LowPart);
		};

		std::vector<Win32::MonitorInformation> newMonitorInfoArr{};

		for (const auto& pathInfo : pathInfoArr | std::views::filter(filterPathInfoLambda))
		{
			if ((pathInfo.flags & DISPLAYCONFIG_PATH_SUPPORT_VIRTUAL_MODE) != 0) [[likely]]
			{
				assert(pathInfo.sourceInfo.sourceModeInfoIdx < modeInfoArr.size());
				assert(modeInfoArr[pathInfo.sourceInfo.sourceModeInfoIdx].infoType == DISPLAYCONFIG_MODE_INFO_TYPE::DISPLAYCONFIG_MODE_INFO_TYPE_SOURCE);

				DISPLAYCONFIG_SOURCE_MODE& srcMode{ modeInfoArr[pathInfo.sourceInfo.sourceModeInfoIdx].sourceMode };

				assert(pathInfo.targetInfo.targetModeInfoIdx < modeInfoArr.size());
				assert(modeInfoArr[pathInfo.targetInfo.targetModeInfoIdx].infoType == DISPLAYCONFIG_MODE_INFO_TYPE::DISPLAYCONFIG_MODE_INFO_TYPE_TARGET);

				DISPLAYCONFIG_TARGET_MODE& targetMode{ modeInfoArr[pathInfo.targetInfo.targetModeInfoIdx].targetMode };

				assert(pathInfo.targetInfo.desktopModeInfoIdx < modeInfoArr.size());
				assert(modeInfoArr[pathInfo.targetInfo.desktopModeInfoIdx].infoType == DISPLAYCONFIG_MODE_INFO_TYPE::DISPLAYCONFIG_MODE_INFO_TYPE_DESKTOP_IMAGE);

				DISPLAYCONFIG_DESKTOP_IMAGE_INFO& desktopImageInfo{ modeInfoArr[pathInfo.targetInfo.desktopModeInfoIdx].desktopImageInfo };

				newMonitorInfoArr.push_back(Win32::MonitorInformation{
					.SourceMode{ std::move(srcMode) },
					.TargetMode{ std::move(targetMode) },
					.DesktopImageInfo{ std::move(desktopImageInfo) }
				});
			}
			else [[unlikely]]
			{
				assert(pathInfo.sourceInfo.modeInfoIdx < modeInfoArr.size());
				assert(modeInfoArr[pathInfo.sourceInfo.modeInfoIdx].infoType == DISPLAYCONFIG_MODE_INFO_TYPE::DISPLAYCONFIG_MODE_INFO_TYPE_SOURCE);

				DISPLAYCONFIG_SOURCE_MODE& srcMode{ modeInfoArr[pathInfo.sourceInfo.modeInfoIdx].sourceMode };

				assert(pathInfo.targetInfo.modeInfoIdx < modeInfoArr.size());
				assert(modeInfoArr[pathInfo.targetInfo.modeInfoIdx].infoType == DISPLAYCONFIG_MODE_INFO_TYPE::DISPLAYCONFIG_MODE_INFO_TYPE_TARGET);

				DISPLAYCONFIG_TARGET_MODE& targetMode{ modeInfoArr[pathInfo.targetInfo.modeInfoIdx].targetMode };

				newMonitorInfoArr.push_back(Win32::MonitorInformation{
					.SourceMode{std::move(srcMode)},
					.TargetMode{std::move(targetMode)},
					.DesktopImageInfo{}
				});
			}
		}

		{
			std::scoped_lock<std::mutex> lock{ mCritSection };

			mMonitorInfoArr = std::move(newMonitorInfoArr);

			// Retain all of the previously registered Monitor instances.
			std::vector<const Monitor*> monitorPtrArr{};
			monitorPtrArr.reserve(mMonitorMap.size());

			for (const auto [monitorPtr, oldMonitorInfoPtr] : mMonitorMap)
				monitorPtrArr.push_back(monitorPtr);

			// Clear out the map to remove all of the garbage pointers to Win32::MonitorInformation
			// instances, and then re-assign each Monitor instance back to the map.
			mMonitorMap.clear();

			for (const auto monitorPtr : monitorPtrArr)
				AssignMonitorToMap(*monitorPtr);
		}
	}

	const Win32::MonitorInformation& MonitorInfoGraph::GetMonitorInformation(const Monitor& monitor) const
	{
		std::scoped_lock<std::mutex> lock{ mCritSection };

		assert(mMonitorMap.contains(&monitor) && mMonitorMap.at(&monitor) != nullptr);
		return *(mMonitorMap.at(&monitor));
	}

	void MonitorInfoGraph::AssignMonitorToMap(const Monitor& monitor)
	{
		// *LOCKED*
		//
		// This function is called from a locked context.

		const Brawler::DXGI_OUTPUT_DESC& monitorOutputDesc{ monitor.GetOutputDescription() };

		for (const auto& info : mMonitorInfoArr)
		{
			// Determine which Win32::MonitorInformation instance is relevant to this Monitor instance
			// based on the desktop coordinates of the monitor.
			const DISPLAYCONFIG_SOURCE_MODE& srcMode{ info.SourceMode };

			if (monitorOutputDesc.DesktopCoordinates.left == srcMode.position.x && monitorOutputDesc.DesktopCoordinates.top == srcMode.position.y)
			{
				mMonitorMap.try_emplace(&monitor, &info);
				return;
			}
		}

		assert(false && "ERROR: No Win32::MonitorInformation instance could be found for a Monitor instance!");
	}
}