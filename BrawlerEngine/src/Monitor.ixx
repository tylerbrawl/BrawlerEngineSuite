module;
#include <memory>
#include <DxDef.h>

export module Brawler.Monitor;
import Brawler.AppWindow;
import Brawler.WindowDisplayMode;
import Brawler.Math.MathTypes;

export namespace Brawler
{
	class Monitor
	{
	public:
		explicit Monitor(Microsoft::WRL::ComPtr<Brawler::DXGIOutput>&& dxgiOutputPtr);

		Monitor(const Monitor& rhs) = delete;
		Monitor& operator=(const Monitor& rhs) = delete;

		Monitor(Monitor&& rhs) noexcept = default;
		Monitor& operator=(Monitor&& rhs) noexcept = default;

		Brawler::DXGIOutput& GetDXGIOutput() const;
		HMONITOR GetMonitorHandle() const;

		const Brawler::DXGI_OUTPUT_DESC& GetOutputDescription() const;

		DXGI_FORMAT GetPreferredSwapChainFormat() const;

		void AssignWindow(std::unique_ptr<AppWindow>&& appWindowPtr);
		void ResetWindow();

		void SpawnWindowForMonitor();

		bool HasWindow() const;

		AppWindow& GetAppWindow();
		const AppWindow& GetAppWindow() const;

	private:
		Microsoft::WRL::ComPtr<Brawler::DXGIOutput> mDXGIOutputPtr;
		Brawler::DXGI_OUTPUT_DESC mOutputDesc;

		/// <summary>
		/// We expect each Monitor instance to own at most one AppWindow instance. This is
		/// why we do not store a std::vector of AppWindow instances.
		/// </summary>
		std::unique_ptr<AppWindow> mAppWindowPtr;
	};
}