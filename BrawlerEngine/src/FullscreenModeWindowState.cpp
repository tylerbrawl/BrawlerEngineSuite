module;
#include <DxDef.h>
#include <DirectXMath/DirectXMath.h>

module Brawler.FullscreenModeWindowState;
import Brawler.AppWindow;
import Brawler.Monitor;
import Brawler.SettingID;
import Brawler.SettingsManager;
import Util.General;
import Util.Engine;

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

		// Verify the sanity of the values which we took from the configuration file.
		{
			if (desiredModeDesc.Width == 0 || desiredModeDesc.Height == 0) [[unlikely]]
			{
				desiredModeDesc.Width = 0;
				desiredModeDesc.Height = 0;
			}

			if (desiredModeDesc.RefreshRate.Numerator == 0 || desiredModeDesc.RefreshRate.Denominator == 0) [[unlikely]]
			{
				desiredModeDesc.RefreshRate.Numerator = 0;
				desiredModeDesc.RefreshRate.Denominator = 0;
			}
		}
		
		Brawler::DXGI_MODE_DESC bestModeDesc{};
		Util::General::CheckHRESULT(GetAppWindow().GetOwningMonitor().GetDXGIOutput().FindClosestMatchingMode1(
			&desiredModeDesc,
			&bestModeDesc,
			&(Util::Engine::GetDXGIDevice())
		));

		return bestModeDesc;
	}
}