module;
#include <memory>
#include <unordered_map>
#include "DxDef.h"

export module Brawler.ResourceTransitionRequestManager;

export namespace Brawler
{
	class I_GPUResource;
	class GPUResourceHandle;
	class ResourceTransitionToken;
}

export namespace Brawler
{
	struct ResourceTransitionInfo
	{
		D3D12_RESOURCE_STATES BeforeState;
		D3D12_RESOURCE_STATES AfterState;
	};

	class ResourceTransitionRequestManager
	{
	private:
		struct ResourceTransitionRequest
		{
			D3D12_RESOURCE_STATES BeforeState;
			D3D12_RESOURCE_STATES AfterState;
			std::unique_ptr<ResourceTransitionRequest> ChildNode;

#ifdef _DEBUG
			std::uint64_t RequestID;
#endif // _DEBUG
		};

	public:
		ResourceTransitionRequestManager();

		ResourceTransitionRequestManager(const ResourceTransitionRequestManager& rhs) = delete;
		ResourceTransitionRequestManager& operator=(const ResourceTransitionRequestManager& rhs) = delete;

		ResourceTransitionRequestManager(ResourceTransitionRequestManager&& rhs) noexcept = default;
		ResourceTransitionRequestManager& operator=(ResourceTransitionRequestManager&& rhs) noexcept = default;

		ResourceTransitionToken AddRequest(GPUResourceHandle& hResource, const D3D12_RESOURCE_STATES desiredState);
		
		/// <summary>
		/// Initializes the BeforeState of every ResourceTransitionRequest within the ResourceTransitionRequestManager.
		/// In addition, this function also sets the tracked resource state of every resource transitioned by this
		/// ResourceTransitionRequestManager to be the final state specified by calls to ResourceTransitionRequestManager::AddRequest().
		/// 
		/// In order to prevent race conditions, this should be called by the master render thread.
		/// </summary>
		void InitializeResourceTransitions();

		ResourceTransitionInfo ConsumeResourceTransitionToken(ResourceTransitionToken&& transitionToken);

	private:
		std::unordered_map<I_GPUResource*, std::unique_ptr<ResourceTransitionRequest>> mRequestHeadNodeMap;
	};
}