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
}