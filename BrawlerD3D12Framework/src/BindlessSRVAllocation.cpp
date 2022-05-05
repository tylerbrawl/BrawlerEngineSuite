module;
#include <cstdint>
#include <optional>
#include <cassert>

module Brawler.D3D12.BindlessSRVAllocation;
import Brawler.D3D12.GPUResourceBindlessSRVManager;
import Brawler.D3D12.BindlessSRVSentinel;

namespace Brawler
{
	namespace D3D12
	{
		BindlessSRVAllocation::BindlessSRVAllocation(GPUResourceBindlessSRVManager& srvManager, BindlessSRVSentinel& associatedSentinel) :
			mSRVManagerPtr(&srvManager),
			mAssociatedSentinelPtr(&associatedSentinel)
		{}

		BindlessSRVAllocation::~BindlessSRVAllocation()
		{
			NotifyManagerToReturnSRV();
		}

		BindlessSRVAllocation::BindlessSRVAllocation(BindlessSRVAllocation&& rhs) noexcept :
			mSRVManagerPtr(rhs.mSRVManagerPtr),
			mAssociatedSentinelPtr(rhs.mAssociatedSentinelPtr)
		{
			rhs.mSRVManagerPtr = nullptr;
			rhs.mAssociatedSentinelPtr = nullptr;
		}

		BindlessSRVAllocation& BindlessSRVAllocation::operator=(BindlessSRVAllocation&& rhs) noexcept
		{
			NotifyManagerToReturnSRV();

			mSRVManagerPtr = rhs.mSRVManagerPtr;
			rhs.mSRVManagerPtr = nullptr;

			mAssociatedSentinelPtr = rhs.mAssociatedSentinelPtr;
			rhs.mAssociatedSentinelPtr = nullptr;

			return *this;
		}

		std::uint32_t BindlessSRVAllocation::GetBindlessSRVIndex() const
		{
			assert(mAssociatedSentinelPtr != nullptr && "ERROR: An attempt was made to get the bindless SRV index for a BindlessSRVAllocation, but it was never associated with a BindlessSRVSentinel instance!");
			return (mAssociatedSentinelPtr->GetBindlessSRVIndex());
		}

		void BindlessSRVAllocation::NotifyManagerToReturnSRV()
		{
			if (mSRVManagerPtr != nullptr && mAssociatedSentinelPtr != nullptr)
			{
				mSRVManagerPtr->ReturnBindlessSRV(*mAssociatedSentinelPtr);

				mSRVManagerPtr = nullptr;
				mAssociatedSentinelPtr = nullptr;
			}
		}
	}
}