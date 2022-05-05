module;
#include <vector>
#include <optional>

export module Brawler.D3D12.DeletePageableGPUObjectState;
import Brawler.D3D12.I_FreeGPUResidencyState;

export namespace Brawler
{
	namespace D3D12
	{
		class DeletePageableGPUObjectState final : public I_FreeGPUResidencyState<DeletePageableGPUObjectState>
		{
		public:
			DeletePageableGPUObjectState() = default;

			FreeGPUResidencyResult TryFreeResidency(FreeGPUResidencyInfo& freeInfo);

			std::optional<FreeGPUResidencyStateID> GetFallbackStateID();
			std::optional<FreeGPUResidencyStateID> GetFallbackStateID() const;

		private:
			void CacheDeleteableObjects(const FreeGPUResidencyInfo& freeInfo);
			void DeleteObjects(FreeGPUResidencyInfo& freeInfo);

		private:
			std::optional<std::vector<I_PageableGPUObject*>> mDeleteableObjectArr;
		};
	}
}