module;
#include <span>
#include <algorithm>
#include <cassert>
#include "DxDef.h"

module Brawler.D3D12.GPUResourceAccessManager;
import Util.D3D12;

namespace Brawler
{
	namespace D3D12
	{
		void GPUResourceAccessManager::SetCurrentResourceDependencies(const std::span<const FrameGraphResourceDependency> dependencySpan)
		{
#ifdef _DEBUG
			mCurrDependencySpan = dependencySpan;
#endif // _DEBUG
		}

		void GPUResourceAccessManager::ClearCurrentResourceDependencies()
		{
#ifdef _DEBUG
			mCurrDependencySpan = std::span<const FrameGraphResourceDependency>{};
#endif // _DEBUG
		}

		bool GPUResourceAccessManager::IsResourceAccessValid(const I_GPUResource& resource, const D3D12_RESOURCE_STATES requiredState) const
		{
#ifdef _DEBUG
			// Stupid COMMON underlying value...
			if (requiredState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON)
				return (std::ranges::find_if(mCurrDependencySpan, [&resource, requiredState] (const FrameGraphResourceDependency& dependency) { return ((dependency.ResourcePtr == &resource) && (dependency.RequiredState == requiredState)); }) != mCurrDependencySpan.end());
			else
				return (std::ranges::find_if(mCurrDependencySpan, [&resource, requiredState] (const FrameGraphResourceDependency& dependency) { return ((dependency.ResourcePtr == &resource) && ((dependency.RequiredState & requiredState) == requiredState)); }) != mCurrDependencySpan.end());
#else
			return true;
#endif // _DEBUG
		}
	}
}