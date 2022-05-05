module;
#include <vector>
#include <memory>
#include <span>
#include <cassert>
#include <variant>
#include "DxDef.h"

module Brawler.D3D12.ResourceStateZoneOptimizer;
import Brawler.D3D12.GPUResourceStateManagement;
import Brawler.D3D12.ReadResourceStateZoneOptimizerState;
import Brawler.D3D12.IgnoreResourceStateZoneOptimizerState;
import Util.D3D12;

namespace Brawler
{
	namespace D3D12
	{
		ResourceStateZoneOptimizer::ResourceStateZoneOptimizer(const I_GPUResource& resource) :
			mCurrState(IgnoreResourceStateZoneOptimizerState{ *this }),
			mStateZonesToDeleteArr(),
			mRequestedStateChange(),
			mResource(&resource)
		{}

		void ResourceStateZoneOptimizer::ProcessResourceStateZone(ResourceStateZone& stateZone)
		{
			mCurrState.AccessData([&stateZone]<typename OptimizerState_T>(OptimizerState_T& optimizerState)
			{
				optimizerState.ProcessResourceStateZone(stateZone);
			});

			ChangeOptimizerState();
		}

		void ResourceStateZoneOptimizer::OnStateDecayBarrier()
		{
			mCurrState.AccessData([]<typename OptimizerState_T>(OptimizerState_T& optimizerState)
			{
				optimizerState.OnStateDecayBarrier();
			});

			ChangeOptimizerState();
		}

		std::span<ResourceStateZone* const> ResourceStateZoneOptimizer::GetResourceStateZonesToDelete() const
		{
			return std::span<ResourceStateZone* const>{ mStateZonesToDeleteArr };
		}

		void ResourceStateZoneOptimizer::AddResourceStateZoneForDeletion(ResourceStateZone& stateZone)
		{
			mStateZonesToDeleteArr.push_back(&stateZone);
		}

		void ResourceStateZoneOptimizer::RequestOptimizerStateChange(StateChangeParams&& stateChangeParams)
		{
			mRequestedStateChange = std::move(stateChangeParams);
		}

		const I_GPUResource& ResourceStateZoneOptimizer::GetGPUResource() const
		{
			return *mResource;
		}

		void ResourceStateZoneOptimizer::ChangeOptimizerState()
		{
			if (mRequestedStateChange.has_value())
			{
				switch (mRequestedStateChange->StateID)
				{
				case ResourceStateZoneOptimizerStateID::READ_RESOURCE_STATE_ZONE:
				{
					mCurrState = ReadResourceStateZoneOptimizerState{ *(mRequestedStateChange->StateZone), *this };
					break;
				}

				case ResourceStateZoneOptimizerStateID::IGNORE_RESOURCE_STATE_ZONE:
				{
					mCurrState = IgnoreResourceStateZoneOptimizerState{ *this };
					break;
				}

				default:
				{
					assert(false);
					std::unreachable();

					break;
				}
				}

				mRequestedStateChange.reset();
			}
		}
	}
}