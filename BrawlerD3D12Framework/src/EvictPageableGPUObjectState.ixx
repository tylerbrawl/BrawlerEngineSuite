module;
#include <vector>
#include <span>
#include <optional>
#include "DxDef.h"

export module Brawler.D3D12.EvictPageableGPUObjectState;
import Brawler.D3D12.I_FreeGPUResidencyState;

export namespace Brawler
{
	namespace D3D12
	{
		class EvictPageableGPUObjectState final : public I_FreeGPUResidencyState<EvictPageableGPUObjectState>
		{
		public:
			EvictPageableGPUObjectState() = default;

			FreeGPUResidencyResult TryFreeResidency(FreeGPUResidencyInfo& freeInfo);

			std::optional<FreeGPUResidencyStateID> GetFallbackStateID();
			std::optional<FreeGPUResidencyStateID> GetFallbackStateID() const;

		private:
			void CacheEvictableObjects(const FreeGPUResidencyInfo& freeInfo);
			std::vector<I_PageableGPUObject*> GetObjectsToEvict(FreeGPUResidencyInfo& freeInfo);
			HRESULT EvictObjects(const std::span<I_PageableGPUObject* const> pageableObjectSpan) const;

		private:
			std::optional<std::vector<I_PageableGPUObject*>> mCachedEvictableObjectArr;
		};
	}
}