module;
#include <atomic>

module Brawler.D3D12.BindlessGPUResourceGroupRegistration;

namespace Brawler
{
	namespace D3D12
	{
		BindlessGPUResourceGroupRegistration::BindlessGPUResourceGroupRegistration() :
			mIsValid(true)
		{}

		void BindlessGPUResourceGroupRegistration::MarkAsInvalidated()
		{
			mIsValid.store(false, std::memory_order::relaxed);
		}

		bool BindlessGPUResourceGroupRegistration::IsValid() const
		{
			return mIsValid.load(std::memory_order::relaxed);
		}
	}
}