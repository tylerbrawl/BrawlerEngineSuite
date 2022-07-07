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