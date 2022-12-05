module;
#include <vector>
#include <memory>
#include <unordered_map>
#include <DxDef.h>

export module Brawler.MonitorHub;
import Brawler.Monitor;
import Brawler.WindowDisplayMode;
import Brawler.AppWindow;
import Brawler.MonitorInfoGraph;
import Brawler.Math.MathTypes;

export namespace Brawler
{
	class MonitorHub final
	{
	private:
		MonitorHub();

	public:
		~MonitorHub() = default;

		MonitorHub(const MonitorHub& rhs) = delete;
		MonitorHub& operator=(const MonitorHub& rhs) = delete;

		MonitorHub(MonitorHub&& rhs) noexcept = delete;
		MonitorHub& operator=(MonitorHub&& rhs) noexcept = delete;

		static MonitorHub& GetInstance();

		MonitorInfoGraph& GetMonitorInfoGraph();
		const MonitorInfoGraph& GetMonitorInfoGraph() const;

		const Monitor& GetMonitorFromPoint(const Math::Int2 desktopCoords) const;
		const Monitor& GetMonitorFromWindow(const AppWindow& window) const;

	private:
		void EnumerateDisplayOutputs();

		Monitor& GetPrimaryMonitor();
		const Monitor& GetPrimaryMonitor() const;

	private:
		std::vector<std::unique_ptr<Monitor>> mMonitorArr;
		MonitorInfoGraph mMonitorInfoGraph;
	};
}