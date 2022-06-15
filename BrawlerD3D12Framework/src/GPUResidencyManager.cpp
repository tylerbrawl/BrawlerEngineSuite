module;
#include <algorithm>
#include <optional>
#include <vector>
#include <ranges>
#include <span>
#include <cassert>
#include "DxDef.h"

module Brawler.D3D12.GPUResidencyManager;
import Brawler.D3D12.I_PageableGPUObject;
import Util.Engine;
import Util.D3D12;
import Util.General;
import Brawler.D3D12.GPUMemoryBudgetInfo;
import Brawler.D3D12.GPUCapabilities;
import Brawler.PolymorphicAdapter;
import Brawler.FreeGPUResidencyStateTraits;
import Brawler.D3D12.I_FreeGPUResidencyState;
import Brawler.D3D12.EvictPageableGPUObjectState;
import Brawler.D3D12.DeletePageableGPUObjectState;

namespace
{
	static constexpr float DESIRED_BUDGET_USAGE_THRESHOLD = 0.8f;

	Brawler::PolymorphicAdapter<Brawler::D3D12::I_FreeGPUResidencyState> CreateFreeGPUResidencyState(const Brawler::D3D12::FreeGPUResidencyStateID stateID)
	{
		using namespace Brawler::D3D12;
		
		switch (stateID)
		{
		case FreeGPUResidencyStateID::EVICT_PAGEABLE_GPU_OBJECT:
			return Brawler::PolymorphicAdapter<I_FreeGPUResidencyState>{ EvictPageableGPUObjectState{} };

		case FreeGPUResidencyStateID::DELETE_PAGEABLE_GPU_OBJECT:
			return Brawler::PolymorphicAdapter<I_FreeGPUResidencyState>{ DeletePageableGPUObjectState{} };

		default:
			assert(false);
			std::unreachable();

			return Brawler::PolymorphicAdapter<I_FreeGPUResidencyState>{};
		}
	}
}

namespace Brawler
{
	namespace D3D12
	{
		void GPUResidencyManager::RegisterPageableGPUObject(I_PageableGPUObject& object)
		{
			mPageableObjArr.PushBack(&object);
		}

		void GPUResidencyManager::UnregisterPageableGPUObject(I_PageableGPUObject& object)
		{
			mPageableObjArr.EraseIf([&object] (I_PageableGPUObject* const objPtr) { return (objPtr == &object); });
		}

		GPUResidencyManager::ResidencyPassResults GPUResidencyManager::ExecuteResidencyPass() const
		{
			// Get the I_PageableGPUObject instances which need to be made resident right now.
			const std::vector<I_PageableGPUObject*> objectsToMakeResidentArr{ GetEvictedObjectsNeedingResidency() };

			// Try to make these objects resident immediately, if it is possible.
			std::span<I_PageableGPUObject* const> objectsToMakeResidentSpan{ objectsToMakeResidentArr };
			ResidencyPassResults makeResidentPassResults{ TryMakeResident(objectsToMakeResidentSpan) };

			GPUMemoryBudgetInfo budgetInfo{ Util::D3D12::GetGPUMemoryBudgetInfo() };
			float currentBudgetThreshold = DESIRED_BUDGET_USAGE_THRESHOLD;
			Brawler::PolymorphicAdapter<I_FreeGPUResidencyState> freeResidencyState{};

			{
				const std::uint64_t originalBudget = budgetInfo.DeviceLocalMemoryInfo.Budget;

				const GPUCapabilities& deviceCapabilities{ Util::Engine::GetGPUCapabilities() };
				const bool isEvictionHelpful = (deviceCapabilities.MaxGPUVirtualAddressBitsPerProcess > originalBudget);

				if (isEvictionHelpful) [[likely]]
					freeResidencyState = EvictPageableGPUObjectState{};
				else
					freeResidencyState = DeletePageableGPUObjectState{};
			}

			// Execute the loop for freeing GPU residency.
			while (budgetInfo.DeviceLocalMemoryInfo.CurrentUsage > budgetInfo.DeviceLocalMemoryInfo.Budget) [[unlikely]]
			{
				FreeGPUResidencyInfo freeResidencyInfo{
					.PageableObjectArr{ mPageableObjArr },
					.BudgetInfo{ budgetInfo }
				};

				// Modify the budget given to the I_FreeGPUResidencyState instance to account
				// for our own target budget.
				{
					DXGI_QUERY_VIDEO_MEMORY_INFO& memoryInfo{ freeResidencyInfo.BudgetInfo.DeviceLocalMemoryInfo };
					memoryInfo.Budget = static_cast<std::uint64_t>(static_cast<float>(memoryInfo.Budget) * currentBudgetThreshold);
				}
				
				// Trim down the memory usage, either by evicting objects from GPU memory or by
				// outright deleting them. Specifically, the D3D12 API states that it is only
				// worthwhile to evict resources if the GPU virtual address range per-process is
				// large enough (i.e., isEvictionHelpful == true).
				const FreeGPUResidencyResult freeResidencyResult = freeResidencyState.AccessData([&freeResidencyInfo]<typename T>(I_FreeGPUResidencyState<T>& state)
				{
					return state.TryFreeResidency(freeResidencyInfo);
				});

				if (freeResidencyResult == FreeGPUResidencyResult::SUCCESS)
				{
					// It is possible that our previous attempt(s) to make the evicted objects needed
					// for the current frame resident failed. In that case, we should try again, now
					// that we have freed up some more memory.
					if (makeResidentPassResults.HResult != S_OK)
						makeResidentPassResults = TryMakeResident(objectsToMakeResidentSpan);
				}
				else
				{
					// If we could not free any residency in the current I_FreeGPUResidencyState, then
					// we check for a fallback method. If none exists, then we are unfortunately out of
					// options; otherwise, we keep going.

					const std::optional<FreeGPUResidencyStateID> fallbackStateID{ freeResidencyState.AccessData([]<typename T>(I_FreeGPUResidencyState<T>& state)
					{
						return state.GetFallbackStateID();
					})};

					if (fallbackStateID.has_value())
						freeResidencyState = CreateFreeGPUResidencyState(*fallbackStateID);
					else
						break;
				}
				
				// Get the updated budget info.
				budgetInfo = Util::D3D12::GetGPUMemoryBudgetInfo();
			}

			// If we did not resolve all of the page faults, then the program cannot continue, as we
			// have run out of memory.
			if (makeResidentPassResults.HResult != S_OK) [[unlikely]]
				throw std::runtime_error{ "ERROR: The GPU does not have enough memory to make all of the resources needed for the current frame resident!" };

			return makeResidentPassResults;
		}

		std::vector<I_PageableGPUObject*> GPUResidencyManager::GetEvictedObjectsNeedingResidency() const
		{
			std::vector<I_PageableGPUObject*> evictedObjectArr{};

			mPageableObjArr.ForEach([&evictedObjectArr] (I_PageableGPUObject* const objPtr)
			{
				if (!objPtr->IsResident() && objPtr->NeedsResidency())
					evictedObjectArr.push_back(objPtr);
			});

			return evictedObjectArr;
		}

		GPUResidencyManager::MakeResidentResults GPUResidencyManager::TryMakeResident(std::span<I_PageableGPUObject* const>& pageableObjectSpan) const
		{
			// Under most scenarios, we shouldn't actually have to do any eviction.
			if (pageableObjectSpan.empty()) [[likely]]
				return MakeResidentResults{
					.MakeResidentFence{},
					.HResult = S_OK
				};
			
			std::vector<ID3D12Pageable*> d3dPageableArr{};
			d3dPageableArr.reserve(pageableObjectSpan.size());

			for (const auto pageableObjectPtr : pageableObjectSpan)
				d3dPageableArr.push_back(&(pageableObjectPtr->GetD3D12PageableObject()));

			Microsoft::WRL::ComPtr<Brawler::D3D12Fence> d3dFence{};

			{
				Microsoft::WRL::ComPtr<ID3D12Fence> oldFenceVersion{};
				Util::General::CheckHRESULT(Util::Engine::GetD3D12Device().CreateFence(
					0,
					D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE,
					IID_PPV_ARGS(&oldFenceVersion)
				));

				Util::General::CheckHRESULT(oldFenceVersion.As(&d3dFence));
			}
			
			// ID3D12Device3::EnqueueMakeResident() will asynchronously make the specified ID3D12Pageable
			// objects resident, rather than calling the CPU to stall. We want it to fail only if we have
			// truly run out of memory; if we are over budget, then we will handle that afterwards.

			const HRESULT hr = Util::Engine::GetD3D12Device().EnqueueMakeResident(
				D3D12_RESIDENCY_FLAGS::D3D12_RESIDENCY_FLAG_NONE,
				static_cast<std::uint32_t>(d3dPageableArr.size()),
				d3dPageableArr.data(),
				d3dFence.Get(),
				1
			);

			switch (hr)
			{
			case S_OK:
			{
				// If we succeeded, then update the residency status of each I_PageableGPUObject and clear
				// the provided std::span.
				for (const auto pageableObjectPtr : pageableObjectSpan)
					pageableObjectPtr->SetResidencyStatus(true);

				pageableObjectSpan = std::span<I_PageableGPUObject* const>{};

				GPUFence makeResidentFence{};
				makeResidentFence.Initialize(std::move(d3dFence), 1);

				return MakeResidentResults{
					.MakeResidentFence{std::move(makeResidentFence)},
					.HResult = S_OK
				};
			}

			case E_OUTOFMEMORY:
			{
				return MakeResidentResults{
					.MakeResidentFence{},
					.HResult = E_OUTOFMEMORY
				};
			}

			default:
			{
				Util::General::CheckHRESULT(hr);

				assert(false);
				std::unreachable();

				return MakeResidentResults{};
			}
			}
		}
	}
}