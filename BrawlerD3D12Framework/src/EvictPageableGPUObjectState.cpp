module;
#include <vector>
#include <ranges>
#include <algorithm>
#include <span>
#include <optional>
#include <cassert>
#include "DxDef.h"

module Brawler.D3D12.EvictPageableGPUObjectState;
import Brawler.D3D12.I_PageableGPUObject;
import Brawler.D3D12.GPUMemoryBudgetInfo;
import Util.Engine;

namespace
{
	enum class EvictionStrategy
	{
		TRIM_TO_BUDGET,
		EVICT_ALL_POSSIBLE_OBJECTS
	};

	static constexpr EvictionStrategy EVICTION_STRATEGY = EvictionStrategy::TRIM_TO_BUDGET;
}

namespace Brawler
{
	namespace D3D12
	{
		FreeGPUResidencyResult EvictPageableGPUObjectState::TryFreeResidency(FreeGPUResidencyInfo& freeInfo)
		{
			// Rather than extracting and sorting the objects to evict each time we call this
			// function, since we know that the list of I_PageableGPUObject instances will remain
			// the same for the entire time this EvictPageableGPUObjectState instance is alive,
			// we can cache the objects which need to be made resident and just use those.

			if (!mCachedEvictableObjectArr.has_value())
				CacheEvictableObjects(freeInfo);

			if (mCachedEvictableObjectArr->empty())
				return FreeGPUResidencyResult::ERROR_CANNOT_FREE_OBJECTS;
			
			const std::vector<I_PageableGPUObject*> objectsToEvictArr{ GetObjectsToEvict(freeInfo) };
			return (SUCCEEDED(EvictObjects(std::span<I_PageableGPUObject* const>{ objectsToEvictArr })) ? FreeGPUResidencyResult::SUCCESS : FreeGPUResidencyResult::ERROR_CANNOT_FREE_OBJECTS);
		}

		std::optional<FreeGPUResidencyStateID> EvictPageableGPUObjectState::GetFallbackStateID()
		{
			// If we cannot evict any more objects, then fall back to deletion.
			return std::optional<FreeGPUResidencyStateID>{ FreeGPUResidencyStateID::DELETE_PAGEABLE_GPU_OBJECT };
		}

		std::optional<FreeGPUResidencyStateID> EvictPageableGPUObjectState::GetFallbackStateID() const
		{
			// If we cannot evict any more objects, then fall back to deletion.
			return std::optional<FreeGPUResidencyStateID>{ FreeGPUResidencyStateID::DELETE_PAGEABLE_GPU_OBJECT };
		}

		void EvictPageableGPUObjectState::CacheEvictableObjects(const FreeGPUResidencyInfo& freeInfo)
		{
			mCachedEvictableObjectArr = std::vector<I_PageableGPUObject*>{};
			
			freeInfo.PageableObjectArr.ForEach([this] (I_PageableGPUObject* const objectPtr)
			{
				if (!(objectPtr->NeedsResidency()))
					mCachedEvictableObjectArr->push_back(objectPtr);
			});

			if constexpr (EVICTION_STRATEGY != EvictionStrategy::EVICT_ALL_POSSIBLE_OBJECTS)
			{
				// Sort the objects which we can evict in the most optimal order. Specifically, we want
				// to evict objects based on the frequency at which they are used, followed by their
				// size.
				//
				// NOTE: This sorter goes in reverse order so that we can make use of functions
				// like std::vector::pop_back().
				std::ranges::sort(*mCachedEvictableObjectArr, [] (const I_PageableGPUObject* const lhs, const I_PageableGPUObject* const rhs)
				{
					// First, sort by usage frequency. We want objects which are used least frequently
					// to be evicted first in order to keep stuttering to a minimum.
					static constexpr float USAGE_FREQUENCY_EPSILON = 0.01f;

					if (std::abs(lhs->GetUsageMetric() - rhs->GetUsageMetric()) > USAGE_FREQUENCY_EPSILON)
						return (rhs->GetUsageMetric() < lhs->GetUsageMetric());

					// If those are (approximately) equivalent, then we sort by size. We want to reduce
					// the number of evictions, so we prefer to evict larger resources over smaller
					// ones.
					return (lhs->GetGPUMemorySize() < rhs->GetGPUMemorySize());
				});
			}
		}

		std::vector<I_PageableGPUObject*> EvictPageableGPUObjectState::GetObjectsToEvict(FreeGPUResidencyInfo& freeInfo)
		{
			if constexpr (EVICTION_STRATEGY == EvictionStrategy::TRIM_TO_BUDGET)
			{
				std::vector<I_PageableGPUObject*> objectsToEvictArr{};
				DXGI_QUERY_VIDEO_MEMORY_INFO& memoryInfo{ freeInfo.BudgetInfo.DeviceLocalMemoryInfo };

				while (memoryInfo.CurrentUsage > memoryInfo.Budget && !(mCachedEvictableObjectArr->empty()))
				{
					I_PageableGPUObject* const objectToEvictPtr = mCachedEvictableObjectArr->back();
					memoryInfo.CurrentUsage -= objectToEvictPtr->GetGPUMemorySize();

					objectsToEvictArr.push_back(objectToEvictPtr);
					mCachedEvictableObjectArr->pop_back();
				}

				return objectsToEvictArr;
			}

			else if constexpr (EVICTION_STRATEGY == EvictionStrategy::EVICT_ALL_POSSIBLE_OBJECTS)
				return std::move(*mCachedEvictableObjectArr);

			else
			{
				assert(false);
				std::unreachable();

				return std::vector<I_PageableGPUObject*>{};
			}
		}

		HRESULT EvictPageableGPUObjectState::EvictObjects(const std::span<I_PageableGPUObject* const> pageableObjectSpan) const
		{
			if (pageableObjectSpan.empty())
				return S_OK;
			
			std::vector<ID3D12Pageable*> d3dPageableArr{};
			d3dPageableArr.reserve(pageableObjectSpan.size());

			for (const auto pageableObjectPtr : pageableObjectSpan)
				d3dPageableArr.push_back(&(pageableObjectPtr->GetD3D12PageableObject()));

			const HRESULT hr = Util::Engine::GetD3D12Device().Evict(static_cast<std::uint32_t>(d3dPageableArr.size()), d3dPageableArr.data());

			if (SUCCEEDED(hr)) [[likely]]
			{
				for (const auto pageableObjectPtr : pageableObjectSpan)
					pageableObjectPtr->SetResidencyStatus(false);
			}

			return hr;
		}
	}
}