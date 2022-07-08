module;
#include <cassert>
#include <DxDef.h>

module Brawler.Monitor;
import Util.General;

namespace Brawler
{
	Monitor::Monitor(Microsoft::WRL::ComPtr<Brawler::DXGIOutput>&& dxgiOutputPtr) :
		mDXGIOutputPtr(std::move(dxgiOutputPtr)),
		mOutputDesc()
	{
		assert(mDXGIOutputPtr != nullptr);
		Util::General::CheckHRESULT(mDXGIOutputPtr->GetDesc1(&mOutputDesc));
	}

	Brawler::DXGIOutput& Monitor::GetDXGIOutput() const
	{
		return *(mDXGIOutputPtr.Get());
	}

	HMONITOR Monitor::GetMonitorHandle() const
	{
		return mOutputDesc.Monitor;
	}

	const Brawler::DXGI_OUTPUT_DESC& Monitor::GetOutputDescription() const
	{
		return mOutputDesc;
	}

	DXGI_FORMAT Monitor::GetPreferredSwapChainFormat() const
	{
		// D3D12 only supports the flip-model style of presentation, which in turn only supports
		// three back buffer formats:
		//
		//   - DXGI_FORMAT_R16G16B16A16_FLOAT
		//   - DXGI_FORMAT_B8G8R8A8_UNORM
		//   - DXGI_FORMAT_R8G8B8A8_UNORM
		//
		// Note that the _SRGB formats (where applicable) are not included in this list. However,
		// for render target views created for back buffers only, it is legal to convert from these
		// formats to their respective _SRGB variant, should they have one.

		switch (mOutputDesc.ColorSpace)
		{
		case DXGI_COLOR_SPACE_TYPE::DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709:
			return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;

		case DXGI_COLOR_SPACE_TYPE::DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020:
			return DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;

		default: [[unlikely]]
		{
			// If we find an unknown color space, then I guess we can just use the same format
			// as with HDR Rec.2020.
			return DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		}
		}
	}

	void Monitor::AssignWindow(std::unique_ptr<AppWindow>&& appWindowPtr)
	{
		assert(mAppWindowPtr == nullptr && "ERROR: An attempt was made to call Monitor::AssignWindow() on a Monitor instance which already had a window!");

		mAppWindowPtr = std::move(appWindowPtr);
		mAppWindowPtr->SetOwningMonitor(*this);
	}

	void Monitor::ResetWindow()
	{
		mAppWindowPtr.reset();
	}

	void Monitor::SpawnWindowForMonitor()
	{
		assert(mAppWindowPtr != nullptr);
		mAppWindowPtr->SpawnWindow();
	}
}