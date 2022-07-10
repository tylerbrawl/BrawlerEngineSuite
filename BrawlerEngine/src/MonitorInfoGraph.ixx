module;
#include <vector>
#include <unordered_map>
#include <mutex>

export module Brawler.MonitorInfoGraph;
import Brawler.Win32.MonitorInformation;
import Brawler.Monitor;

export namespace Brawler
{
	class MonitorInfoGraph
	{
	public:
		MonitorInfoGraph() = default;

		MonitorInfoGraph(const MonitorInfoGraph& rhs) = delete;
		MonitorInfoGraph& operator=(const MonitorInfoGraph& rhs) = delete;

		MonitorInfoGraph(MonitorInfoGraph&& rhs) noexcept = delete;
		MonitorInfoGraph& operator=(MonitorInfoGraph&& rhs) noexcept = delete;

		void RegisterMonitor(const Monitor& monitor);
		void UpdateCachedMonitorInformation();

		const Win32::MonitorInformation& GetMonitorInformation(const Monitor& monitor) const;

	private:
		void AssignMonitorToMap(const Monitor& monitor);

	private:
		std::vector<Win32::MonitorInformation> mMonitorInfoArr;
		std::unordered_map<const Monitor*, const Win32::MonitorInformation*> mMonitorMap;
		mutable std::mutex mCritSection;
	};
}