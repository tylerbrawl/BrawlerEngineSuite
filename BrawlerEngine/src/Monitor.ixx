module;
#include <memory>
#include <vector>
#include <span>
#include <DxDef.h>

export module Brawler.Monitor;
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

		void UpdateMonitorInformation();

		const Brawler::DXGI_OUTPUT_DESC& GetOutputDescription() const;

		DXGI_FORMAT GetPreferredSwapChainFormat() const;

		std::span<const Brawler::DXGI_MODE_DESC> GetDisplayModeSpan() const;

	private:
		void ResetDisplayModeList();

	private:
		Microsoft::WRL::ComPtr<Brawler::DXGIOutput> mDXGIOutputPtr;
		Brawler::DXGI_OUTPUT_DESC mOutputDesc;

		/// <summary>
		/// This describes the list of display modes supported by this monitor. Strictly
		/// speaking, according to the MSDN, it is possible, but rare, for the list to
		/// change at runtime. To keep things simple, however, we will just cache the list
		/// which we find at the creation of the Monitor instance.
		/// 
		/// Keep in mind that the display modes are relevant for fullscreen swap chains.
		/// </summary>
		std::vector<Brawler::DXGI_MODE_DESC> mDisplayModeArr;
	};
}