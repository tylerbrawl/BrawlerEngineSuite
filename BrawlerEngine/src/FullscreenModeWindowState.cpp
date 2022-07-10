module;
#include <vector>
#include <span>
#include <ranges>
#include <algorithm>
#include <DxDef.h>
#include <DirectXMath/DirectXMath.h>

module Brawler.FullscreenModeWindowState;
import Brawler.AppWindow;
import Brawler.Monitor;
import Brawler.SettingID;
import Brawler.SettingsManager;
import Util.General;
import Util.Engine;
import Brawler.MonitorHub;
import Brawler.MonitorInfoGraph;
import Brawler.Win32.MonitorInformation;

namespace Brawler
{
	FullscreenModeWindowState::FullscreenModeWindowState(AppWindow& owningWnd) :
		I_WindowState<FullscreenModeWindowState>(owningWnd)
	{}

	Win32::WindowMessageResult FullscreenModeWindowState::ProcessWindowMessage(const Win32::WindowMessage& msg)
	{
		return Win32::UnhandledMessageResult();
	}

	Win32::CreateWindowInfo FullscreenModeWindowState::GetCreateWindowInfo() const
	{
		constexpr std::uint32_t WINDOW_STYLE_FULLSCREEN_MODE = 0;
		constexpr std::uint32_t WINDOW_STYLE_EX_FULLSCREEN_MODE = 0;

		const Brawler::DXGI_OUTPUT_DESC& currMonitorDesc{ GetAppWindow().GetOwningMonitor().GetOutputDescription() };

		DirectX::XMINT2 windowStartCoordinates{
			currMonitorDesc.DesktopCoordinates.left,
			currMonitorDesc.DesktopCoordinates.top
		};

		const Brawler::DXGI_MODE_DESC bestFullscreenMode{ GetBestFullscreenMode() };

		DirectX::XMUINT2 windowSize{
			bestFullscreenMode.Width,
			bestFullscreenMode.Height
		};

		return Win32::CreateWindowInfo{
			.WindowStyle = WINDOW_STYLE_FULLSCREEN_MODE,
			.WindowStyleEx = WINDOW_STYLE_EX_FULLSCREEN_MODE,
			.WindowStartCoordinates{ std::move(windowStartCoordinates) },
			.WindowSize{ std::move(windowSize) }
		};
	}

	SwapChainCreationInfo FullscreenModeWindowState::GetSwapChainCreationInfo() const
	{
		const Brawler::DXGI_OUTPUT_DESC& currMonitorDesc{ GetAppWindow().GetOwningMonitor().GetOutputDescription() };
		const Brawler::DXGI_MODE_DESC bestFullscreenMode{ GetBestFullscreenMode() };

		Brawler::DXGI_SWAP_CHAIN_DESC swapChainDesc{ GetDefaultSwapChainDescription() };
		swapChainDesc.Width = bestFullscreenMode.Width;
		swapChainDesc.Height = bestFullscreenMode.Height;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullscreenDesc{
			.RefreshRate{ bestFullscreenMode.RefreshRate },
			.ScanlineOrdering = bestFullscreenMode.ScanlineOrdering,
			.Scaling = bestFullscreenMode.Scaling,
			.Windowed = FALSE
		};

		return SwapChainCreationInfo{
			.HWnd = GetAppWindow().GetWindowHandle(),
			.SwapChainDesc{ std::move(swapChainDesc) },
			.FullscreenDesc{ std::move(swapChainFullscreenDesc) }
		};
	}

	void FullscreenModeWindowState::OnShowWindow()
	{}

	Brawler::DXGI_MODE_DESC FullscreenModeWindowState::GetBestFullscreenMode() const
	{
		Brawler::DXGI_MODE_DESC desiredModeDesc{
			.Width = Brawler::SettingsManager::GetInstance().GetOption<SettingID::FULLSCREEN_RESOLUTION_WIDTH>(),
			.Height = Brawler::SettingsManager::GetInstance().GetOption<SettingID::FULLSCREEN_RESOLUTION_HEIGHT>(),
			.RefreshRate{
				.Numerator = Brawler::SettingsManager::GetInstance().GetOption<SettingID::FULLSCREEN_REFRESH_RATE_NUMERATOR>(),
				.Denominator = Brawler::SettingsManager::GetInstance().GetOption<SettingID::FULLSCREEN_REFRESH_RATE_DENOMINATOR>()
			},
			.Format = GetAppWindow().GetOwningMonitor().GetPreferredSwapChainFormat(),
			.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER::DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
			.Scaling = DXGI_MODE_SCALING::DXGI_MODE_SCALING_UNSPECIFIED,
			.Stereo = FALSE
		};

		// Unfortunately, we cannot use IDXGIOutput1::FindClosestMatchingMode1 because ID3D12Device (along with all of its
		// derived classes) does not support the IDXGIOutput1 interface; that is, QueryInterface() cannot be used to get
		// an IDXGIOutput1 instance from a D3D12 device.
		//
		// This means that we need to implement a search ourselves.

		const std::span<const Brawler::DXGI_MODE_DESC> monitorDisplayModeSpan{ GetAppWindow().GetOwningMonitor().GetDisplayModeSpan() };
		std::vector<const Brawler::DXGI_MODE_DESC*> modesForDesiredFormatArr{};

		{
			const auto filterDisplayModesLambda = [&desiredModeDesc] (const Brawler::DXGI_MODE_DESC& currModeDesc)
			{
				if (desiredModeDesc.ScanlineOrdering != DXGI_MODE_SCANLINE_ORDER::DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED && desiredModeDesc.ScanlineOrdering != currModeDesc.ScanlineOrdering) [[unlikely]]
					return false;

				if (desiredModeDesc.Scaling != DXGI_MODE_SCALING::DXGI_MODE_SCALING_UNSPECIFIED && desiredModeDesc.Scaling != currModeDesc.Scaling) [[unlikely]]
					return false;

				if (desiredModeDesc.Stereo != currModeDesc.Stereo) [[unlikely]]
					return false;

				return (desiredModeDesc.Format == currModeDesc.Format);
			};

			for (const auto& displayMode : monitorDisplayModeSpan | std::views::filter(filterDisplayModesLambda))
				modesForDesiredFormatArr.push_back(&displayMode);
		}

		// Sort the display modes from most to least desirable.
		std::ranges::sort(modesForDesiredFormatArr, [&desiredModeDesc] (const Brawler::DXGI_MODE_DESC* lhs, const Brawler::DXGI_MODE_DESC* rhs)
		{
			// First, compare the display modes based on resolution. If the resolution width and/or height is 0,
			// then the highest resolution is preferred. Otherwise, the display mode with the lowest difference in
			// resolution from the desired display mode is the better choice.
			if(desiredModeDesc.Width != 0 && desiredModeDesc.Height != 0) [[likely]]
			{
				const std::uint32_t lhsWidthDifference = std::abs(static_cast<std::int32_t>(desiredModeDesc.Width) - static_cast<std::int32_t>(lhs->Width));
				const std::uint32_t rhsWidthDifference = std::abs(static_cast<std::int32_t>(desiredModeDesc.Width) - static_cast<std::int32_t>(rhs->Width));

				if (lhsWidthDifference != rhsWidthDifference)
					return (lhsWidthDifference < rhsWidthDifference);

				const std::uint32_t lhsHeightDifference = std::abs(static_cast<std::int32_t>(desiredModeDesc.Height) - static_cast<std::int32_t>(lhs->Height));
				const std::uint32_t rhsHeightDifference = std::abs(static_cast<std::int32_t>(desiredModeDesc.Height) - static_cast<std::int32_t>(rhs->Height));

				if (lhsHeightDifference != rhsHeightDifference)
					return (lhsHeightDifference < rhsHeightDifference);
			}
			else [[unlikely]]
			{
				// We found an invalid resolution in the configuration file, so just choose based on the highest
				// possible resolution.
				if (lhs->Width != rhs->Width)
					return (lhs->Width > rhs->Width);

				if (lhs->Height != rhs->Height)
					return (lhs->Height > rhs->Height);
			}

			// If they both have the same resolution, then we continue sorting based on refresh rate. If the
			// refresh rate numerator and/or denominator is 0, then the highest refresh rate is preferred. Otherwise,
			// the display mode with the lowest difference in refresh rate from the desired refresh rate is the better
			// choice.
			const float lhsRefreshRate = static_cast<float>(lhs->RefreshRate.Numerator) / static_cast<float>(lhs->RefreshRate.Denominator);
			const float rhsRefreshRate = static_cast<float>(rhs->RefreshRate.Numerator) / static_cast<float>(rhs->RefreshRate.Denominator);

			if (desiredModeDesc.RefreshRate.Numerator != 0 && desiredModeDesc.RefreshRate.Denominator != 0 && lhs->RefreshRate.Numerator != rhs->RefreshRate.Numerator &&
				lhs->RefreshRate.Denominator != rhs->RefreshRate.Denominator) [[likely]]
			{
				// Ensure that we are using valid values, since the refresh rate was taken from the configuration file.
				const float desiredRefreshRate = static_cast<float>(desiredModeDesc.RefreshRate.Numerator) / static_cast<float>(desiredModeDesc.RefreshRate.Denominator);

				const float lhsRefreshRateDifference = std::abs(desiredRefreshRate - lhsRefreshRate);
				const float rhsRefreshRateDifference = std::abs(desiredRefreshRate - rhsRefreshRate);

				return (lhsRefreshRateDifference < rhsRefreshRateDifference);
			}
			else [[unlikely]]
				return (lhsRefreshRate > rhsRefreshRate);

			// Taking into consideration the initial filtering function, if both display modes have the same resolution
			// and refresh rate, then they should be equally as desirable. This shouldn't happen unless 
			// IDXGIOutput1::GetDisplayModeList1() somehow produces duplicate entries.
		});

		if (!modesForDesiredFormatArr.empty()) [[likely]]
			return *(modesForDesiredFormatArr[0]);

		// In the unlikely event that this failed, then we should fallback to the system settings for the
		// current Monitor.
		const DISPLAYCONFIG_VIDEO_SIGNAL_INFO& currMonitorSignalInfo{ MonitorHub::GetInstance().GetMonitorInfoGraph().GetMonitorInformation(GetAppWindow().GetOwningMonitor()).TargetMode.targetVideoSignalInfo };
		desiredModeDesc.Width = currMonitorSignalInfo.activeSize.cx;
		desiredModeDesc.Height = currMonitorSignalInfo.activeSize.cy;
		desiredModeDesc.RefreshRate.Numerator = currMonitorSignalInfo.vSyncFreq.Numerator;
		desiredModeDesc.RefreshRate.Denominator = currMonitorSignalInfo.vSyncFreq.Denominator;

		switch (currMonitorSignalInfo.scanLineOrdering)
		{
		case DISPLAYCONFIG_SCANLINE_ORDERING::DISPLAYCONFIG_SCANLINE_ORDERING_UNSPECIFIED:
		{
			desiredModeDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER::DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
			break;
		}

		case DISPLAYCONFIG_SCANLINE_ORDERING::DISPLAYCONFIG_SCANLINE_ORDERING_PROGRESSIVE:
		{
			desiredModeDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER::DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
			break;
		}

		case DISPLAYCONFIG_SCANLINE_ORDERING::DISPLAYCONFIG_SCANLINE_ORDERING_INTERLACED_UPPERFIELDFIRST:
		{
			desiredModeDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER::DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST;
			break;
		}

		case DISPLAYCONFIG_SCANLINE_ORDERING::DISPLAYCONFIG_SCANLINE_ORDERING_INTERLACED_LOWERFIELDFIRST:
		{
			desiredModeDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER::DXGI_MODE_SCANLINE_ORDER_LOWER_FIELD_FIRST;
			break;
		}

		default: [[unlikely]]
		{
			assert(false);
			std::unreachable();

			break;
		}
		}

		return desiredModeDesc;
	}
}