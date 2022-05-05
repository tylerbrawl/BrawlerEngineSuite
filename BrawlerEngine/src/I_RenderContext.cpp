module;
#include <cassert>
#include "DxDef.h"

module Brawler.I_RenderContext;
import Util.Engine;
import Brawler.GPUResourceHandle;
import Util.General;
import Brawler.ResourceTransitionRequestManager;
import Brawler.I_GPUResource;
import Brawler.I_RenderJob;

namespace
{
	bool IsResourcePromotableFromCommonState(const Brawler::I_GPUResource& resource)
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
	I_RenderContext::I_RenderContext(const Brawler::CommandListType cmdListType) :
		mRecordingJob(nullptr),
		mCmdAllocator(nullptr),
		mCmdList(nullptr),
		mPendingBarriers()
	{
		assert(cmdListType != CommandListType::COUNT_OR_ERROR && "ERROR: An invalid Brawler::CommandListType was specified when constructing an I_RenderContext instance!");

		// Create the command allocator. 
		// 
		// Again, the MSDN is incorrect. It states that the type of the command allocator has to be either 
		// that for direct command lists or that for bundles. In reality, however, when they said "direct" 
		// command lists, they really meant whatever type of command list the allocator is going to be managing 
		// (i.e., direct, compute, or copy).
		//
		// Is anybody else fed up with Direct3D 12's awful documentation?
		CheckHRESULT(Util::Engine::GetD3D12Device().CreateCommandAllocator(Brawler::GetD3D12CommandListType(cmdListType), IID_PPV_ARGS(&mCmdAllocator)));

		// Create the command list.
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList{ nullptr };
		CheckHRESULT(Util::Engine::GetD3D12Device().CreateCommandList(
			0,
			Brawler::GetD3D12CommandListType(cmdListType),
			mCmdAllocator.Get(),
			nullptr,
			IID_PPV_ARGS(&cmdList)
		));

		CheckHRESULT(cmdList.As(&mCmdList));
	}

	void I_RenderContext::TransitionResource(ResourceTransitionToken&& transitionToken)
	{
		mPendingBarriers.push_back(std::move(transitionToken));
	}

	void I_RenderContext::TransitionResourceImmediate(ResourceTransitionToken&& transitionToken)
	{
		I_GPUResource* const resourcePtr{ transitionToken.mResource };
		ResourceTransitionInfo transitionInfo{ mRecordingJob->mTransitionManager.ConsumeResourceTransitionToken(std::move(transitionToken)) };

		// Silently drop resource transitions if the resource is currently in and promotable from
		// the D3D12_RESOURCE_STATE_COMMON state.
		if (IsResourcePromotableFromCommonState(*resourcePtr) && transitionInfo.BeforeState == D3D12_RESOURCE_STATE_COMMON)
			return;

		D3D12_RESOURCE_BARRIER transition{ CD3DX12_RESOURCE_BARRIER::Transition(&(resourcePtr->GetD3D12Resource()), transitionInfo.BeforeState, transitionInfo.AfterState) };
		mCmdList->ResourceBarrier(1, &transition);
	}

	void I_RenderContext::ExecuteResourceBarriers()
	{
		if (mPendingBarriers.empty())
			return;
		
		std::vector<D3D12_RESOURCE_BARRIER> barrierArr{};
		barrierArr.reserve(mPendingBarriers.size());

		for (auto&& transitionToken : mPendingBarriers)
		{
			I_GPUResource* const resourcePtr{ transitionToken.mResource };
			ResourceTransitionInfo transitionInfo{ mRecordingJob->mTransitionManager.ConsumeResourceTransitionToken(std::move(transitionToken)) };

			// Silently drop resource transitions if the resource is currently in and promotable from
			// the D3D12_RESOURCE_STATE_COMMON state.
			if (IsResourcePromotableFromCommonState(*resourcePtr) && transitionInfo.BeforeState == D3D12_RESOURCE_STATE_COMMON)
				continue;
			
			barrierArr.push_back(CD3DX12_RESOURCE_BARRIER::Transition(&(resourcePtr->GetD3D12Resource()), transitionInfo.BeforeState, transitionInfo.AfterState));
		}

		if (!barrierArr.empty())
			mCmdList->ResourceBarrier(static_cast<std::uint32_t>(barrierArr.size()), barrierArr.data());

		mPendingBarriers.clear();
	}

	void I_RenderContext::CloseCommandList()
	{
		ExecuteResourceBarriers();
		CheckHRESULT(mCmdList->Close());

		mRecordingJob = nullptr;
	}

	void I_RenderContext::ResetCommandList()
	{
		CheckHRESULT(mCmdAllocator->Reset());
		CheckHRESULT(mCmdList->Reset(mCmdAllocator.Get(), nullptr));
	}

	void I_RenderContext::SetRecordingJob(I_RenderJob& renderJob)
	{
		mRecordingJob = &renderJob;
	}

	Brawler::D3D12GraphicsCommandList& I_RenderContext::GetCommandList()
	{
		return *(mCmdList.Get());
	}

	const Brawler::D3D12GraphicsCommandList& I_RenderContext::GetCommandList() const
	{
		return *(mCmdList.Get());
	}

	bool I_RenderContext::VerifyResourceHandleAccess(const GPUResourceHandle& hResource, const ResourceAccessMode requiredAccessMode) const
	{
#ifdef _DEBUG
		return (hResource.GetOwningJob() == mRecordingJob && Util::General::EnumCast(requiredAccessMode) <= Util::General::EnumCast(hResource.GetAccessMode()));
#else
		return true;
#endif // _DEBUG
	}
}