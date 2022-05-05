module;
#include <optional>

export module Brawler.D3D12.I_FreeGPUResidencyState;
import Brawler.D3D12.FreeGPUResidencyInfo;
import Brawler.D3D12.FreeGPUResidencyStateID;

export namespace Brawler
{
	namespace D3D12
	{
		class GPUResidencyManager;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		template <typename DerivedType>
		class I_FreeGPUResidencyState
		{
		protected:
			I_FreeGPUResidencyState() = default;

		public:
			virtual ~I_FreeGPUResidencyState() = default;

			FreeGPUResidencyResult TryFreeResidency(FreeGPUResidencyInfo& freeInfo);

			std::optional<FreeGPUResidencyStateID> GetFallbackStateID();
			std::optional<FreeGPUResidencyStateID> GetFallbackStateID() const;
		};
	}
}

// ------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <typename DerivedType>
		FreeGPUResidencyResult I_FreeGPUResidencyState<DerivedType>::TryFreeResidency(FreeGPUResidencyInfo& freeInfo)
		{
			return static_cast<DerivedType*>(this)->TryFreeResidency(freeInfo);
		}

		template <typename DerivedType>
		std::optional<FreeGPUResidencyStateID> I_FreeGPUResidencyState<DerivedType>::GetFallbackStateID()
		{
			return static_cast<DerivedType*>(this)->GetFallbackStateID();
		}

		template <typename DerivedType>
		std::optional<FreeGPUResidencyStateID> I_FreeGPUResidencyState<DerivedType>::GetFallbackStateID() const
		{
			return static_cast<DerivedType*>(this)->GetFallbackStateID();
		}
	}
}