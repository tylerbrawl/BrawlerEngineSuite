module;
#include <vector>
#include <memory>
#include <DxDef.h>

module Brawler.MonitorHub;
import Util.Engine;
import Util.General;

namespace Brawler
{
	MonitorHub::MonitorHub() :
		mMonitorArr()
	{
		EnumerateDisplayOutputs();
	}

	MonitorHub& MonitorHub::GetInstance()
	{
		static MonitorHub instance{};
		return instance;
	}

	void MonitorHub::EnumerateDisplayOutputs()
	{
		Brawler::DXGIAdapter& dxgiAdapter{ Util::Engine::GetDXGIAdapter() };

		// Get the first adapter. If this fails, then we throw an exception, because there
		// should be at least one connected adapter.
		{
			Microsoft::WRL::ComPtr<IDXGIOutput> oldDXGIOutputPtr{};
			const HRESULT hr = dxgiAdapter.EnumOutputs(0, &oldDXGIOutputPtr);

			if (FAILED(hr)) [[unlikely]]
				throw std::runtime_error{ "ERROR: A display output could not be found!" };

			Microsoft::WRL::ComPtr<Brawler::DXGIOutput> newDXGIOutputPtr{};
			Util::General::CheckHRESULT(oldDXGIOutputPtr.As(&newDXGIOutputPtr));

			mMonitorArr.push_back(std::make_unique<Monitor>(std::move(newDXGIOutputPtr)));
		}

		// Continue checking for additional monitors.
		std::uint32_t currIndex = 1;
		bool doMoreOutputsExist = true;

		while (doMoreOutputsExist)
		{
			Microsoft::WRL::ComPtr<IDXGIOutput> oldDXGIOutputPtr{};
			const HRESULT hr = dxgiAdapter.EnumOutputs(currIndex++, &oldDXGIOutputPtr);

			switch (hr)
			{
			// S_OK: The operation completed successfully (i.e., we found another monitor).
			case S_OK:
			{
				Microsoft::WRL::ComPtr<Brawler::DXGIOutput> newDXGIOutputPtr{};
				Util::General::CheckHRESULT(oldDXGIOutputPtr.As(&newDXGIOutputPtr));

				mMonitorArr.push_back(std::make_unique<Monitor>(std::move(newDXGIOutputPtr)));

				break;
			}

			// DXGI_ERROR_NOT_FOUND: currIndex has exceeded the number of connected outputs. In this
			// case, we have found all of the monitors, and can thus exit.
			case DXGI_ERROR_NOT_FOUND:
			{
				doMoreOutputsExist = false;
				break;
			}

			default: [[unlikely]]
			{
				Util::General::CheckHRESULT(hr);

				doMoreOutputsExist = false;
				break;
			}
			}
		}
	}
}