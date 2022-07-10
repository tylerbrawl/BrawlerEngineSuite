module;
#include <cassert>
#include <DxDef.h>
#include <DirectXMath/DirectXMath.h>

export module Brawler.I_WindowState;
import Brawler.Win32.WindowMessage;
import Brawler.Win32.CreateWindowInfo;
import Brawler.SwapChain;
import Brawler.Monitor;
import Util.Engine;

export namespace Brawler
{
	class AppWindow;
}

export namespace Brawler
{
	template <typename DerivedType>
	class I_WindowState
	{
	protected:
		explicit I_WindowState(AppWindow& owningWnd);

	public:
		virtual ~I_WindowState() = default;

		I_WindowState(const I_WindowState& rhs) = delete;
		I_WindowState& operator=(const I_WindowState& rhs) = delete;

		I_WindowState(I_WindowState&& rhs) noexcept = default;
		I_WindowState& operator=(I_WindowState&& rhs) noexcept = default;

		Win32::WindowMessageResult ProcessWindowMessage(const Win32::WindowMessage& msg);
		void OnShowWindow();

		Win32::CreateWindowInfo GetCreateWindowInfo() const;
		SwapChainCreationInfo GetSwapChainCreationInfo() const;

	protected:
		AppWindow& GetAppWindow();
		const AppWindow& GetAppWindow() const;

		Brawler::DXGI_SWAP_CHAIN_DESC GetDefaultSwapChainDescription() const;

	private:
		AppWindow* mOwningWndPtr;
	};
}

// ---------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename DerivedType>
	I_WindowState<DerivedType>::I_WindowState(AppWindow& owningWnd) :
		mOwningWndPtr(&owningWnd)
	{}

	template <typename DerivedType>
	Win32::WindowMessageResult I_WindowState<DerivedType>::ProcessWindowMessage(const Win32::WindowMessage& msg)
	{
		return static_cast<DerivedType*>(this)->ProcessWindowMessage(msg);
	}

	template <typename DerivedType>
	void I_WindowState<DerivedType>::OnShowWindow()
	{
		static_cast<DerivedType*>(this)->OnShowWindow();
	}

	template <typename DerivedType>
	Win32::CreateWindowInfo I_WindowState<DerivedType>::GetCreateWindowInfo() const
	{
		return static_cast<const DerivedType*>(this)->GetCreateWindowInfo();
	}

	template <typename DerivedType>
	SwapChainCreationInfo I_WindowState<DerivedType>::GetSwapChainCreationInfo() const
	{
		return static_cast<const DerivedType*>(this)->GetSwapChainCreationInfo();
	}

	template <typename DerivedType>
	AppWindow& I_WindowState<DerivedType>::GetAppWindow()
	{
		assert(mOwningWndPtr != nullptr);
		return *mOwningWndPtr;
	}

	template <typename DerivedType>
	const AppWindow& I_WindowState<DerivedType>::GetAppWindow() const
	{
		assert(mOwningWndPtr != nullptr);
		return *mOwningWndPtr;
	}

	template <typename DerivedType>
	Brawler::DXGI_SWAP_CHAIN_DESC I_WindowState<DerivedType>::GetDefaultSwapChainDescription() const
	{
		// It is recommended by NVIDIA to create 1-2 more back buffers than you have frames in
		// flight.
		constexpr std::uint32_t SWAP_CHAIN_BUFFER_COUNT = (Util::Engine::MAX_FRAMES_IN_FLIGHT + 1);

		// NOTE: The description of the DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH flag on the MSDN at
		// https://docs.microsoft.com/en-us/windows/win32/api/dxgi/ne-dxgi-dxgi_swap_chain_flag#constants
		// is rather misleading, in my opinion. It gives the impression that if the flag is set, then
		// calling IDXGISwapChain::ResizeTarget() may cause the swap chain to transition from fullscreen 
		// to windowed mode and vice versa.
		//
		// When the description says that the flag enables the application to "switch modes by calling
		// IDXGISwapChain::ResizeTarget()," it is NOT stating that calling this function might result
		// in the swap chain moving from windowed to fullscreen mode or vice versa. Rather, it means
		// that calling this function with the swap chain in fullscreen mode might result in the desktop
		// resolution (or display *mode*) being changed.
		//
		// They try to clarify this in the description's second statement, but the ambiguous use of the
		// term "modes" in the first statement led me to believe that said "modes" were actually referring
		// to the swap between fullscreen and windowed mode. Anyways, I'm just leaving this here in case
		// anybody else was as confused by that as I was.
		DXGI_SWAP_CHAIN_FLAG swapChainFlags = (DXGI_SWAP_CHAIN_FLAG::DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT | DXGI_SWAP_CHAIN_FLAG::DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);

		if (Util::Engine::GetGPUCapabilities().IsTearingAllowed) [[likely]]
			swapChainFlags |= DXGI_SWAP_CHAIN_FLAG::DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

		return Brawler::DXGI_SWAP_CHAIN_DESC{
			.Width{},
			.Height{},
			.Format = GetAppWindow().GetOwningMonitor().GetPreferredSwapChainFormat(),
			.Stereo = FALSE,
			.SampleDesc{
				.Count = 1,
				.Quality = 0
			},
			.BufferUsage = (DXGI_CPU_ACCESS_NONE | DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_UNORDERED_ACCESS),
			.BufferCount = SWAP_CHAIN_BUFFER_COUNT,

			// We always want the swap chain buffer size to be the same as the target output dimensions.
			// According to the D3D12 samples, this is needed in order to get access to true independent
			// flip mode in fullscreen.
			//
			// Even for a windowed mode swap chain description, we still want to do this. That
			// way, we can have a uniform scaling method for all window modes.
			.Scaling = DXGI_SCALING::DXGI_SCALING_NONE,

			.SwapEffect = DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_FLIP_DISCARD,
			.AlphaMode = DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_UNSPECIFIED,
			.Flags = swapChainFlags
		};
	}
}