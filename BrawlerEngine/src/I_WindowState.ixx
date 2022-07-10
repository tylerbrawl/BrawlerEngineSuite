module;
#include <cassert>
#include <optional>
#include <DxDef.h>
#include <DirectXMath/DirectXMath.h>

export module Brawler.I_WindowState;
import Brawler.Win32.WindowMessage;
import Brawler.Win32.CreateWindowInfo;
import Brawler.SwapChain;

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

		I_WindowState(I_WindowState&& rhs) noexcept;
		I_WindowState& operator=(I_WindowState&& rhs) noexcept;

		Win32::WindowMessageResult ProcessWindowMessage(const Win32::WindowMessage& msg);
		void OnShowWindow();

		Win32::CreateWindowInfo GetCreateWindowInfo() const;
		SwapChainCreationInfo GetSwapChainCreationInfo() const;

	protected:
		bool HasAppWindow() const;

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
	Brawler::DXGI_SWAP_CHAIN_DESC GetDefaultSwapChainDescription(const AppWindow& window);
}

namespace Brawler
{
	template <typename DerivedType>
	I_WindowState<DerivedType>::I_WindowState(AppWindow& owningWnd) :
		mOwningWndPtr(&owningWnd)
	{}

	template <typename DerivedType>
	I_WindowState<DerivedType>::I_WindowState(I_WindowState&& rhs) noexcept :
		mOwningWndPtr(rhs.mOwningWndPtr)
	{
		rhs.mOwningWndPtr = nullptr;
	}

	template <typename DerivedType>
	I_WindowState<DerivedType>& I_WindowState<DerivedType>::operator=(I_WindowState<DerivedType>&& rhs) noexcept
	{
		mOwningWndPtr = rhs.mOwningWndPtr;
		rhs.mOwningWndPtr = nullptr;

		return *this;
	}

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
	bool I_WindowState<DerivedType>::HasAppWindow() const
	{
		return (mOwningWndPtr != nullptr);
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
		return Brawler::GetDefaultSwapChainDescription(GetAppWindow());
	}
}