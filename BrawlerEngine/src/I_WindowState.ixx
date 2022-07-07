module;
#include <cassert>
#include <DxDef.h>
#include <DirectXMath/DirectXMath.h>

export module Brawler.I_WindowState;
import Brawler.Win32.WindowMessage;
import Brawler.Win32.CreateWindowInfo;

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
		Win32::CreateWindowInfo GetCreateWindowInfo() const;

	protected:
		AppWindow& GetAppWindow();
		const AppWindow& GetAppWindow() const;

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
	Win32::CreateWindowInfo I_WindowState<DerivedType>::GetCreateWindowInfo() const
	{
		return static_cast<const DerivedType*>(this)->GetCreateWindowInfo();
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
}