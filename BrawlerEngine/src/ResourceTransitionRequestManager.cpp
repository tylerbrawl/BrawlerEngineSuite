module;
#include <memory>
#include <cassert>
#include "DxDef.h"

module Brawler.ResourceTransitionRequestManager;
import Brawler.GPUResourceHandle;
import Util.Engine;
import Brawler.I_GPUResource;
import Brawler.ResourceTransitionToken;

namespace
{
	bool DoesResourceDecayToCommonState(const Brawler::I_GPUResource& resource)
	{
		// The only resources which can benefit from implicit resource decay/promotion
		// are buffers and textures created with the 
		// D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS flags. By recognizing this, we
		// can reduce the number of required resource barriers and significantly
		// improve performance.

		const Brawler::D3D12_RESOURCE_DESC resourceDesc{ resource.GetResourceDescription() };
		return (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER || resourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS);
	}
}

namespace Brawler
{
	ResourceTransitionRequestManager::ResourceTransitionRequestManager() :
		mRequestHeadNodeMap()
	{}

	ResourceTransitionToken ResourceTransitionRequestManager::AddRequest(GPUResourceHandle& hResource, const D3D12_RESOURCE_STATES desiredState)
	{
		I_GPUResource* const resourcePtr = &(*hResource);

		std::unique_ptr<ResourceTransitionRequest> transitionRequest{ std::make_unique<ResourceTransitionRequest>() };
		transitionRequest->AfterState = desiredState;
		
		if (!mRequestHeadNodeMap.contains(resourcePtr))
		{
#ifdef _DEBUG
			transitionRequest->RequestID = 0;
#endif // _DEBUG

			mRequestHeadNodeMap[resourcePtr] = std::move(transitionRequest);
			return ResourceTransitionToken{ *(hResource.mResource), 0 };
		}
			
		else
		{
			ResourceTransitionRequest* requestPtr = mRequestHeadNodeMap[resourcePtr].get();

			while (requestPtr->ChildNode != nullptr)
				requestPtr = requestPtr->ChildNode.get();

			requestPtr->ChildNode = std::move(transitionRequest);

#ifdef _DEBUG
			requestPtr->ChildNode->RequestID = (requestPtr->RequestID + 1);

			return ResourceTransitionToken{ *(hResource.mResource), (requestPtr->RequestID + 1) };
#else
			return ResourceTransitionToken{ *(hResource.mResource), 0 };
#endif // _DEBUG
		}
	}

	void ResourceTransitionRequestManager::InitializeResourceTransitions()
	{
		assert(Util::Engine::IsMasterRenderThread() && "ERROR: An attempt was made to prepare the resource transitions for an I_RenderJob from a thread other than the master render thread! This *WILL* introduce race conditions!");

		for (auto& itr : mRequestHeadNodeMap)
		{
			// Set the BeforeState of the head node to be the current state of the resource.
			itr.second->BeforeState = (itr.first)->GetCurrentResourceState();

			// Iteratively update the BeforeState of each child node to be the AfterState of 
			// its parent node.
			ResourceTransitionRequest* requestPtr = itr.second.get();

			while (requestPtr->ChildNode != nullptr)
			{
				const D3D12_RESOURCE_STATES prevState = requestPtr->AfterState;
				requestPtr = requestPtr->ChildNode.get();
				requestPtr->BeforeState = prevState;
			}

			// If the resource implicitly decays to the D3D12_RESOURCE_STATE_COMMON state,
			// then account for that. Otherwise, set its resource state to be that of the
			// AfterState of the final ResourceTransitionRequest of the linked list.
			bool decays = DoesResourceDecayToCommonState(*(itr.first));
			if (decays)
				(itr.first)->SetCurrentResourceState(D3D12_RESOURCE_STATE_COMMON);
			else
				(itr.first)->SetCurrentResourceState(requestPtr->AfterState);
		}
	}

	ResourceTransitionInfo ResourceTransitionRequestManager::ConsumeResourceTransitionToken(ResourceTransitionToken&& transitionToken)
	{
		assert(mRequestHeadNodeMap.contains(transitionToken.mResource) && mRequestHeadNodeMap[transitionToken.mResource] != nullptr && "ERROR: A ResourceTransitionRequestManager could not fulfill a resource transition request!");

#ifdef _DEBUG
		assert(transitionToken.mRequestID == mRequestHeadNodeMap[transitionToken.mResource]->RequestID && "ERROR: A ResourceTransitionToken was consumed in a order different from that which it was created in!");
#endif // _DEBUG

		ResourceTransitionInfo transitionInfo{};
		transitionInfo.BeforeState = mRequestHeadNodeMap[transitionToken.mResource]->BeforeState;
		transitionInfo.AfterState = mRequestHeadNodeMap[transitionToken.mResource]->AfterState;

		// Update the head node for this resource's transition request linked list.
		mRequestHeadNodeMap[transitionToken.mResource] = std::move(mRequestHeadNodeMap[transitionToken.mResource]->ChildNode);

		// Prevent the ResourceTransitionToken from being re-used.
		transitionToken.mResource = nullptr;

		return transitionInfo;
	}
}