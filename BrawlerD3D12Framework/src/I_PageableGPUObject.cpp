module;

module Brawler.D3D12.I_PageableGPUObject;
import Util.Engine;
import Brawler.D3D12.GPUResidencyManager;

namespace Brawler
{
	namespace D3D12
	{
		I_PageableGPUObject::I_PageableGPUObject() :
			mIsResident(true)
		{
			Util::Engine::GetGPUResidencyManager().RegisterPageableGPUObject(*this);
		}

		I_PageableGPUObject::~I_PageableGPUObject()
		{
			Util::Engine::GetGPUResidencyManager().UnregisterPageableGPUObject(*this);
		}

		void I_PageableGPUObject::SetResidencyStatus(const bool isResident)
		{
			mIsResident = isResident;
		}

		bool I_PageableGPUObject::IsResident() const
		{
			return mIsResident;
		}
	}
}