module;
#include <vector>
#include <memory>
#include <cassert>
#include <DxDef.h>
#include <dwmapi.h>

module Brawler.MonitorHub;
import Util.Engine;
import Util.General;

namespace Brawler
{
	MonitorHub::MonitorHub() :
		mMonitorArr(),
		mMonitorInfoGraph()
	{
		EnumerateDisplayOutputs();

		mMonitorInfoGraph.UpdateCachedMonitorInformation();
		
		for (const auto& monitorPtr : mMonitorArr)
			mMonitorInfoGraph.RegisterMonitor(*monitorPtr);
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

	MonitorInfoGraph& MonitorHub::GetMonitorInfoGraph()
	{
		return mMonitorInfoGraph;
	}

	const MonitorInfoGraph& MonitorHub::GetMonitorInfoGraph() const
	{
		return mMonitorInfoGraph;
	}

	const Monitor& MonitorHub::GetMonitorFromPoint(const Math::Int2 desktopCoords) const
	{
		for (const auto& monitorPtr : mMonitorArr)
		{
			const RECT& monitorDesktopCoords{ monitorPtr->GetOutputDescription().DesktopCoordinates };
			const bool isPointWithinMonitorBounds = (monitorDesktopCoords.left <= desktopCoords.GetX() && monitorDesktopCoords.right > desktopCoords.GetX() &&
				monitorDesktopCoords.top <= desktopCoords.GetY() && monitorDesktopCoords.bottom > desktopCoords.GetY());

			if (isPointWithinMonitorBounds)
				return *monitorPtr;
		}

		return GetPrimaryMonitor();
	}

	const Monitor& MonitorHub::GetMonitorFromWindow(const AppWindow& window) const
	{
		// Get the window's RECT in screen coordinates.
		const RECT windowScreenCoordsRect{ window.GetWindowRect() };

		const Monitor* monitorWithGreatestIntersectionAreaPtr = nullptr;
		std::uint32_t largestIntersectionAreaFound = std::numeric_limits<float>::min();

		for (const auto& monitorPtr : mMonitorArr)
		{
			const RECT& monitorDesktopCoords{ monitorPtr->GetOutputDescription().DesktopCoordinates };
			const RECT intersectionRect{
				.left = std::max(windowScreenCoordsRect.left, monitorDesktopCoords.left),
				.top = std::max(windowScreenCoordsRect.top, monitorDesktopCoords.top),
				.right = std::min(windowScreenCoordsRect.right, monitorDesktopCoords.right),
				.bottom = std::min(windowScreenCoordsRect.bottom, monitorDesktopCoords.bottom)
			};

			// If intersectionRect.left > intersectionRect.right and/or intersectionRect.top > intersectionRect.bottom,
			// then the rectangles do *NOT* intersect.
			if (intersectionRect.left <= intersectionRect.right && intersectionRect.top <= intersectionRect.bottom)
			{
				const std::uint32_t intersectionArea = static_cast<std::uint32_t>(intersectionRect.right - intersectionRect.left) * static_cast<std::uint32_t>(intersectionRect.bottom - intersectionRect.top);

				if (intersectionArea > largestIntersectionAreaFound) [[likely]]
				{
					monitorWithGreatestIntersectionAreaPtr = monitorPtr.get();
					largestIntersectionAreaFound = intersectionArea;
				}
			}
		}

		return (monitorWithGreatestIntersectionAreaPtr != nullptr ? *monitorWithGreatestIntersectionAreaPtr : GetPrimaryMonitor());
	}

	Monitor& MonitorHub::GetPrimaryMonitor()
	{
		assert(!mMonitorArr.empty());
		assert(mMonitorArr[0] != nullptr);

		return *(mMonitorArr[0]);
	}

	const Monitor& MonitorHub::GetPrimaryMonitor() const
	{
		assert(!mMonitorArr.empty());
		assert(mMonitorArr[0] != nullptr);

		return *(mMonitorArr[0]);
	}
}