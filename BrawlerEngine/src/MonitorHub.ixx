module;
#include <vector>
#include <memory>
#include <unordered_map>
#include <DxDef.h>

export module Brawler.MonitorHub;
import Brawler.Monitor;
import Brawler.WindowDisplayMode;
import Brawler.AppWindow;
import Brawler.Win32.WindowMessage;

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

		Win32::WindowMessageResult ProcessWindowMessage(const HWND hWnd, const Win32::WindowMessage& msg);

		void ResetApplicationWindows();

	private:
		void EnumerateDisplayOutputs();

		Monitor& GetPrimaryMonitor();
		const Monitor& GetPrimaryMonitor() const;

		void CreateWindowForMonitor(Monitor& monitor, const WindowDisplayMode displayMode);

	private:
		std::vector<std::unique_ptr<Monitor>> mMonitorArr;
		std::unordered_map<HWND, AppWindow*> mHWndMap;
	};
}