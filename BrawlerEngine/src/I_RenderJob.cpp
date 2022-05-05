module;
#include <cassert>
#include "DxDef.h"

module Brawler.I_RenderJob;
import Util.Engine;

namespace Brawler
{
	I_RenderJob::I_RenderJob() :
#ifdef _DEBUG
		mResourceDependencyMap(),
#endif // _DEBUG
		mTransitionManager()
	{}

	GPUResourceHandle I_RenderJob::AddResourceDependency(I_GPUResource& resource, const Brawler::ResourceAccessMode accessMode)
	{
		GPUResourceHandle hResource{ *this, resource, accessMode };
		
#ifdef _DEBUG
		assert(!mResourceDependencyMap.contains(&resource) && "ERROR: An attempt was made to add a dependency for the same resource to the same I_RenderJob twice!");
		mResourceDependencyMap[&resource] = accessMode;
#endif // _DEBUG
		
		return hResource;
	}

	ResourceTransitionToken I_RenderJob::CreateResourceTransitionToken(GPUResourceHandle& hResource, const D3D12_RESOURCE_STATES desiredState)
	{
		assert(Util::Engine::AreResourceStatesValid(desiredState) && "ERROR: An invalid set of resource states was specified when creating a ResourceTransitionToken for an I_RenderJob instance!");

#ifdef _DEBUG
		assert(hResource.mOwningJob == this && hResource.mAccessMode == Brawler::ResourceAccessMode::WRITE && "ERROR: An attempt was made to create a ResourceTransitionToken without proper privileges!");
#endif // _DEBUG

		return mTransitionManager.AddRequest(hResource, desiredState);
	}

	void I_RenderJob::InitializeResourceTransitionManager()
	{
		mTransitionManager.InitializeResourceTransitions();
	}
}