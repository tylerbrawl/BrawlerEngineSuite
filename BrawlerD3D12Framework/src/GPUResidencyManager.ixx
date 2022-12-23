module;
#include <optional>
#include <vector>
#include <span>

export module Brawler.D3D12.GPUResidencyManager;
import Brawler.ThreadSafeVector;
import Brawler.D3D12.GPUFence;
import Brawler.D3D12.I_PageableGPUObject;
import Brawler.D3D12.GPUMemoryBudgetInfo;

export namespace Brawler
{
	namespace D3D12
	{
		class GPUResidencyManager
		{
		public:
			struct ResidencyPassResults
			{
				std::optional<GPUFence> MakeResidentFence;
				HRESULT HResult;
			};

		private:
			using MakeResidentResults = ResidencyPassResults;

		public:
			GPUResidencyManager() = default;

			GPUResidencyManager(const GPUResidencyManager& rhs) = delete;
			GPUResidencyManager& operator=(const GPUResidencyManager& rhs) = delete;

			GPUResidencyManager(GPUResidencyManager&& rhs) noexcept = default;
			GPUResidencyManager& operator=(GPUResidencyManager&& rhs) noexcept = default;

			void RegisterPageableGPUObject(I_PageableGPUObject& object);
			void UnregisterPageableGPUObject(I_PageableGPUObject& object);

			ResidencyPassResults ExecuteResidencyPass() const;

		private:
			std::vector<I_PageableGPUObject*> GetEvictedObjectsNeedingResidency() const;
			MakeResidentResults TryMakeResident(std::span<I_PageableGPUObject* const>& evictedObjectSpan) const;

		private:
			Brawler::ThreadSafeVector<I_PageableGPUObject*> mPageableObjArr;
		};
	}
}