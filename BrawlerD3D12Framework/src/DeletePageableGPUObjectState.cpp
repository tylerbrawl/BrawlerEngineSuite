module;
#include <vector>
#include <optional>
#include <algorithm>
#include <cassert>
#include "DxDef.h"

module Brawler.D3D12.DeletePageableGPUObjectState;
import Brawler.D3D12.I_PageableGPUObject;

namespace
{
	enum class DeletionStrategy
	{
		TRIM_TO_BUDGET,
		DELETE_ALL_POSSIBLE_OBJECTS
	};

	static constexpr DeletionStrategy DELETION_STRATEGY = DeletionStrategy::TRIM_TO_BUDGET;
}

namespace Brawler
{
	namespace D3D12
	{
		FreeGPUResidencyResult DeletePageableGPUObjectState::TryFreeResidency(FreeGPUResidencyInfo& freeInfo)
		{
			if (!mDeleteableObjectArr.has_value())
				CacheDeleteableObjects(freeInfo);

			if (mDeleteableObjectArr->empty())
				return FreeGPUResidencyResult::ERROR_CANNOT_FREE_OBJECTS;

			DeleteObjects(freeInfo);
			return FreeGPUResidencyResult::SUCCESS;
		}

		std::optional<FreeGPUResidencyStateID> DeletePageableGPUObjectState::GetFallbackStateID()
		{
			return std::optional<FreeGPUResidencyStateID>{};
		}

		std::optional<FreeGPUResidencyStateID> DeletePageableGPUObjectState::GetFallbackStateID() const
		{
			return std::optional<FreeGPUResidencyStateID>{};
		}

		void DeletePageableGPUObjectState::CacheDeleteableObjects(const FreeGPUResidencyInfo& freeInfo)
		{
			mDeleteableObjectArr = std::vector<I_PageableGPUObject*>{};
			
			freeInfo.PageableObjectArr.ForEach([this] (I_PageableGPUObject* const objPtr)
			{
				if (objPtr->IsDeletionSafe())
					mDeleteableObjectArr->push_back(objPtr);
			});

			if constexpr (DELETION_STRATEGY == DeletionStrategy::TRIM_TO_BUDGET)
			{
				// When trimming the memory usage to the budget, we want to delete objects
				// based on their size. Specifically, to get as close to the budget as possible,
				// we want to delete the smallest objects first.
				std::ranges::sort(*mDeleteableObjectArr, [] (const I_PageableGPUObject* const lhs, const I_PageableGPUObject* const rhs)
				{
					return (rhs->GetGPUMemorySize() < lhs->GetGPUMemorySize());
				});
			}
		}

		void DeletePageableGPUObjectState::DeleteObjects(FreeGPUResidencyInfo& freeInfo)
		{
			if constexpr (DELETION_STRATEGY == DeletionStrategy::TRIM_TO_BUDGET)
			{
				DXGI_QUERY_VIDEO_MEMORY_INFO& memoryInfo{ freeInfo.BudgetInfo.DeviceLocalMemoryInfo };

				while (memoryInfo.CurrentUsage > memoryInfo.Budget && !mDeleteableObjectArr->empty())
				{
					I_PageableGPUObject* const objectToDeletePtr = mDeleteableObjectArr->back();
					memoryInfo.CurrentUsage -= objectToDeletePtr->GetGPUMemorySize();

					objectToDeletePtr->DeleteD3D12PageableObject();
					mDeleteableObjectArr->pop_back();
				}
			}

			else if constexpr (DELETION_STRATEGY == DeletionStrategy::DELETE_ALL_POSSIBLE_OBJECTS)
			{
				for (const auto deleteableObjectPtr : *mDeleteableObjectArr)
					deleteableObjectPtr->DeleteD3D12PageableObject();

				mDeleteableObjectArr->clear();
			}

			else
			{
				assert(false);
				std::unreachable();
			}
		}
	}
}