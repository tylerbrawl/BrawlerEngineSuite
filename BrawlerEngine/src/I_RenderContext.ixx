module;
#include <vector>
#include <unordered_map>
#include "DxDef.h"

export module Brawler.I_RenderContext;
import Brawler.CommandListType;
import Brawler.ResourceAccessMode;
import Brawler.ResourceTransitionToken;

export namespace Brawler
{
	class RenderJobManager;
	class I_RenderJob;
	class GPUResourceHandle;
}

export namespace Brawler
{
	class I_RenderContext
	{
	private:
		friend class RenderJobManager;
		friend class I_RenderJob;

	protected:
		explicit I_RenderContext(const Brawler::CommandListType cmdListType);

	public:
		virtual ~I_RenderContext() = default;

		I_RenderContext(const I_RenderContext& rhs) = delete;
		I_RenderContext& operator=(const I_RenderContext& rhs) = delete;

		I_RenderContext(I_RenderContext&& rhs) noexcept = default;
		I_RenderContext& operator=(I_RenderContext&& rhs) noexcept = default;

		/// <summary>
		/// Creates a pending resource transition for the I_GPUResource specified by hResource.
		/// </summary>
		/// <param name="hResource">
		/// - A GPUResourceHandle to the I_GPUResource which is to be transitioned.
		/// </param>
		/// <param name="resourceState">
		/// - The new resource state of the relevant I_GPUResource.
		/// </param>
		void TransitionResource(ResourceTransitionToken&& transitionToken);

		/// <summary>
		/// Immediately sets up a resource barrier to transition the I_GPUResource specified by hResource
		/// to the desired resource state. This function does *NOT* create resource barriers for
		/// the pending resource transitions; to do that, call I_RenderContext::ExecuteResourceBarriers().
		/// </summary>
		/// <param name="hResource">
		/// - A GPUResourceHandle to the I_GPUResource which is to be transitioned.
		/// </param>
		/// <param name="resourceState">
		/// - The new resource state of the relevant I_GPUResource.
		/// </param>
		void TransitionResourceImmediate(ResourceTransitionToken&& transitionToken);

		/// <summary>
		/// Creates a resource barrier for each pending resource transition in the I_RenderContext
		/// instance. This function is automatically called when a command list has finished being
		/// recorded.
		/// </summary>
		void ExecuteResourceBarriers();

		void CloseCommandList();
		void SetRecordingJob(I_RenderJob& renderJob);

	private:
		void ResetCommandList();

	protected:
		Brawler::D3D12GraphicsCommandList& GetCommandList();
		const Brawler::D3D12GraphicsCommandList& GetCommandList() const;

		/// <summary>
		/// This function should be called by member function of I_RenderContext or its derived
		/// classes to ensure that GPUResourceHandles are being used properly.
		/// </summary>
		/// <param name="hResource">
		/// - A GPUResourceHandle whose use is to be verified by this function.
		/// </param>
		/// <param name="requiredAccessMode">
		/// - The minimum access mode which is required for the resource access to be valid.
		/// </param>
		/// <returns>
		/// The function returns true if this I_RenderContext instance has appropriate access to
		/// the specified I_GPUResource and false otherwise.
		/// </returns>
		bool VerifyResourceHandleAccess(const GPUResourceHandle& hResource, const ResourceAccessMode requiredAccessMode) const;

	private:
		I_RenderJob* mRecordingJob;
		Microsoft::WRL::ComPtr<Brawler::D3D12CommandAllocator> mCmdAllocator;
		Microsoft::WRL::ComPtr<Brawler::D3D12GraphicsCommandList> mCmdList;
		std::vector<ResourceTransitionToken> mPendingBarriers;
	};
}