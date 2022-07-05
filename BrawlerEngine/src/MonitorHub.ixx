module;
#include <vector>
#include <memory>
#include <DxDef.h>

export module Brawler.MonitorHub;
import Brawler.Monitor;

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

	private:
		void EnumerateDisplayOutputs();

	private:
		std::vector<std::unique_ptr<Monitor>> mMonitorArr;
	};
}