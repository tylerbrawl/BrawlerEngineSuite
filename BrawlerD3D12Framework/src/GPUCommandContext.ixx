module;
#include <cassert>
#include <functional>
#include <span>
#include "DxDef.h"

export module Brawler.D3D12.GPUCommandContext;
import Brawler.D3D12.GPUCommandQueueType;
import Util.Engine;
import Util.General;
import Brawler.D3D12.GPUResourceAccessManager;
import Brawler.D3D12.GPUFence;
import Brawler.D3D12.FrameGraphResourceDependency;
import Brawler.D3D12.TextureCopyBufferSubAllocation;
import Brawler.D3D12.TextureSubResource;
import Brawler.D3D12.I_BufferSubAllocation;

export namespace Brawler
{
	namespace D3D12
	{
		class DirectContext;
		class ComputeContext;
		class CopyContext;
	}
}

namespace
{
	template <Brawler::D3D12::GPUCommandQueueType CmdListType>
	struct GPUCommandContextInfo
	{
		static_assert(sizeof(CmdListType) != sizeof(CmdListType), "ERROR: An explicit template specialization was not provided for GPUCommandContextInfo with a given Brawler::GPUCommandQueueType! (See GPUCommandContext.ixx.)");
	};

	template <D3D12_COMMAND_LIST_TYPE D3DCmdListType, typename ContextType_>
	struct GPUCommandContextInfoInstantiation
	{
		static constexpr D3D12_COMMAND_LIST_TYPE D3D_CMD_LIST_TYPE = D3DCmdListType;
		
		using ContextType = ContextType_;
	};

	template <>
	struct GPUCommandContextInfo<Brawler::D3D12::GPUCommandQueueType::DIRECT> : public GPUCommandContextInfoInstantiation<D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT, Brawler::D3D12::DirectContext>
	{};

	template <>
	struct GPUCommandContextInfo<Brawler::D3D12::GPUCommandQueueType::COMPUTE> : public GPUCommandContextInfoInstantiation<D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE, Brawler::D3D12::ComputeContext>
	{};

	template <>
	struct GPUCommandContextInfo<Brawler::D3D12::GPUCommandQueueType::COPY> : public GPUCommandContextInfoInstantiation<D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY, Brawler::D3D12::CopyContext>
	{};
}

namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType>
		class GPUCommandQueue;

		class GPUCommandContextVault;

		template <GPUCommandQueueType QueueType>
		class GPUCommandListRecorder;

		template <GPUCommandQueueType QueueType, typename InputDataType>
		class RenderPass;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType CmdListType>
		class GPUCommandContext
		{
		private:
			friend class GPUCommandQueue<CmdListType>;

			friend class GPUCommandContextVault;

			template <GPUCommandQueueType QueueType>
			friend class GPUCommandListRecorder;

			template <GPUCommandQueueType CmdListType, typename InputDataType>
			friend class RenderPass;

		private:
			using DerivedContextType = GPUCommandContextInfo<CmdListType>::ContextType;

		protected:
			GPUCommandContext();

		public:
			virtual ~GPUCommandContext();

			GPUCommandContext(const GPUCommandContext& rhs) = delete;
			GPUCommandContext& operator=(const GPUCommandContext& rhs) = delete;

			GPUCommandContext(GPUCommandContext&& rhs) noexcept = default;
			GPUCommandContext& operator=(GPUCommandContext&& rhs) noexcept = default;

		protected:
			Brawler::D3D12GraphicsCommandList& GetCommandList() const;

		public:
			/// <summary>
			/// Describes whether or not this GPUCommandContext was given any commands to
			/// send to the GPU.
			/// 
			/// NOTE: The GPUCommandContext instance does *NOT* maintain this value itself.
			/// This is set by other objects with the GPUCommandContext::MarkAsUseful()
			/// function.
			/// </summary>
			/// <returns>
			/// The function returns true if it has any commands to submit to the GPU and
			/// false otherwise.
			/// </returns>
			bool HasCommands() const;

		protected:
			bool IsResourceAccessValid(const I_GPUResource& resource, const D3D12_RESOURCE_STATES requiredState) const;

		private:
			/// <summary>
			/// Prior to an I_RenderPass recording its commands into a context, in Debug builds,
			/// it will call this function to set the valid I_GPUResource instances for it. This
			/// prevents unauthorized use of I_GPUResource instances in render passes which do
			/// not have them listed as a resource dependency.
			/// 
			/// In Release builds, this information is ignored.
			/// </summary>
			/// <param name="dependencySpan">
			/// - A std::span referring to a FrameGraphResourceDependency for each I_GPUResource
			///   which an I_RenderPass instance is requesting access to.
			/// </param>
			void SetValidGPUResources(const std::span<const FrameGraphResourceDependency> dependencySpan);

			void RecordCommandList(const std::function<void(DerivedContextType&)>& recordJob);
			virtual void RecordCommandListIMPL(const std::function<void(DerivedContextType&)>& recordJob) = 0;

			/// <summary>
			/// Resets both the command list *AND* its underlying allocator. The latter action
			/// means that this function can only be called *AFTER* the GPU has finished
			/// executing all of the commands of this GPUCommandContext instance.
			/// </summary>
			void ResetCommandList();

			void CloseCommandList();
			void MarkAsUseful();

			/// <summary>
			/// Checks if this GPUCommandContext can be used to record other commands. This
			/// function will return true only if the GPU has finished executing all of the
			/// previous commands.
			/// </summary>
			/// <returns>
			/// The function returns true if the GPU has finished executing all of the commands
			/// of this GPUCommandContext and false otherwise.
			/// </returns>
			bool ReadyForUse() const;

			void SetGPUFence(const GPUFence& fence);
			void ResourceBarrier(const std::span<const CD3DX12_RESOURCE_BARRIER> barrierSpan) const;

			// ==================================================================
			// ^ Implementation Details and General Functions | Shared Commands v
			// ==================================================================

		public:
			void CopyBufferToTexture(const TextureSubResource& destTexture, const TextureCopyBufferSubAllocation& srcSubAllocation) const;
			void CopyTextureToBuffer(const TextureCopyBufferSubAllocation& destSubAllocation, const TextureSubResource& srcTexture) const;

			void CopyBufferToBuffer(const I_BufferSubAllocation& destSubAllocation, const I_BufferSubAllocation& srcSubAllocation) const;

		private:
			Microsoft::WRL::ComPtr<Brawler::D3D12CommandAllocator> mCmdAllocator;
			Microsoft::WRL::ComPtr<Brawler::D3D12GraphicsCommandList> mCmdList;
			GPUResourceAccessManager mResourceAccessManager;
			const GPUFence* mFencePtr;
			std::uint64_t mRequiredFenceValue;
			bool mHasCommands;
		};
	}
}

// ---------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType CmdListType>
		GPUCommandContext<CmdListType>::GPUCommandContext() :
			mCmdAllocator(nullptr),
			mCmdList(nullptr),
			mResourceAccessManager(),
			mFencePtr(nullptr),
			mRequiredFenceValue(0),
			mHasCommands(false)
		{
			Brawler::D3D12Device& d3dDevice{ Util::Engine::GetD3D12Device() };

			{
				// Create the command allocator.
				Util::General::CheckHRESULT(d3dDevice.CreateCommandAllocator(
					GPUCommandContextInfo<CmdListType>::D3D_CMD_LIST_TYPE,
					IID_PPV_ARGS(&mCmdAllocator)
				));
			}

			{
				// Create the command list in the closed state.
				Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList{};
				Util::General::CheckHRESULT(d3dDevice.CreateCommandList1(
					0,
					GPUCommandContextInfo<CmdListType>::D3D_CMD_LIST_TYPE,
					D3D12_COMMAND_LIST_FLAGS::D3D12_COMMAND_LIST_FLAG_NONE,
					IID_PPV_ARGS(&cmdList)
				));

				Util::General::CheckHRESULT(cmdList.As(&mCmdList));
			}
		}

		template <GPUCommandQueueType CmdListType>
		GPUCommandContext<CmdListType>::~GPUCommandContext()
		{
			assert(ReadyForUse() && "ERROR: A command context was destroyed before the GPU could execute all of its commands!");
		}

		template <GPUCommandQueueType CmdListType>
		Brawler::D3D12GraphicsCommandList& GPUCommandContext<CmdListType>::GetCommandList() const
		{
			return *(mCmdList.Get());
		}

		template <GPUCommandQueueType CmdListType>
		bool GPUCommandContext<CmdListType>::HasCommands() const
		{
			return mHasCommands;
		}

		template <GPUCommandQueueType CmdListType>
		bool GPUCommandContext<CmdListType>::IsResourceAccessValid(const I_GPUResource& resource, const D3D12_RESOURCE_STATES requiredState) const
		{
			if constexpr (Util::General::IsDebugModeEnabled())
				return mResourceAccessManager.IsResourceAccessValid(resource, requiredState);
			else
				return true;
		}

		template <GPUCommandQueueType CmdListType>
		void GPUCommandContext<CmdListType>::SetValidGPUResources(const std::span<const FrameGraphResourceDependency> dependencySpan)
		{
			mResourceAccessManager.SetCurrentResourceDependencies(dependencySpan);
		}

		template <GPUCommandQueueType CmdListType>
		void GPUCommandContext<CmdListType>::RecordCommandList(const std::function<void(DerivedContextType&)>& recordJob)
		{
			// Record the commands into the command list.
			RecordCommandListIMPL(recordJob);
		}

		template <GPUCommandQueueType CmdListType>
		void GPUCommandContext<CmdListType>::ResetCommandList()
		{
			assert(ReadyForUse() && "ERROR: An attempt was made to record commands into a command context, but the GPU was not finished executing all of its previous commands!");

			// Reset the command list so that we can record commands into it.
			Util::General::CheckHRESULT(mCmdAllocator->Reset());
			Util::General::CheckHRESULT(mCmdList->Reset(mCmdAllocator.Get(), nullptr));

			// At the beginning of a command list, it has no commands recorded into it.
			mHasCommands = false;
		}

		template <GPUCommandQueueType CmdListType>
		void GPUCommandContext<CmdListType>::CloseCommandList()
		{
			// Close the command list. This lets D3D12 know that we are finished recording commands
			// into it.
			Util::General::CheckHRESULT(mCmdList->Close());
		}

		template <GPUCommandQueueType CmdListType>
		void GPUCommandContext<CmdListType>::MarkAsUseful()
		{
			mHasCommands = true;
		}

		template <GPUCommandQueueType CmdListType>
		bool GPUCommandContext<CmdListType>::ReadyForUse() const
		{
			return (mFencePtr == nullptr || mFencePtr->HasFenceCompletedValue(mRequiredFenceValue));
		}

		template <GPUCommandQueueType CmdListType>
		void GPUCommandContext<CmdListType>::SetGPUFence(const GPUFence& fence)
		{
			mFencePtr = &fence;
			mRequiredFenceValue = fence.GetLastSignalValue();
		}

		template <GPUCommandQueueType CmdListType>
		void GPUCommandContext<CmdListType>::ResourceBarrier(const std::span<const CD3DX12_RESOURCE_BARRIER> barrierSpan) const
		{
			if (!barrierSpan.empty())
				mCmdList->ResourceBarrier(static_cast<std::uint32_t>(barrierSpan.size()), barrierSpan.data());
		}

		// ==================================================================
		// ^ Implementation Details and General Functions | Shared Commands v
		// ==================================================================

		template <GPUCommandQueueType CmdListType>
		void GPUCommandContext<CmdListType>::CopyBufferToTexture(const TextureSubResource& destTexture, const TextureCopyBufferSubAllocation& srcSubAllocation) const
		{
			assert(IsResourceAccessValid(destTexture.GetGPUResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST) && "ERROR: The destination texture resource in a call to GPUCommandContext::CopyBufferToTexture() was not specified as a resource dependency with the D3D12_RESOURCE_STATE_COPY_DEST state!");
			assert(IsResourceAccessValid(srcSubAllocation.GetBufferResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE) && "ERROR: The source buffer resource in a call to GPUCommandContext::CopyBufferToTexture() was not specified as a resource dependency with the D3D12_RESOURCE_STATE_COPY_SOURCE state!");

			const CD3DX12_TEXTURE_COPY_LOCATION destCopyLocation{ &(destTexture.GetD3D12Resource()), destTexture.GetSubResourceIndex() };
			const CD3DX12_TEXTURE_COPY_LOCATION srcCopyLocation{ srcSubAllocation.GetBufferTextureCopyLocation() };

			mCmdList->CopyTextureRegion(
				&destCopyLocation,
				0,
				0,
				0,
				&srcCopyLocation,
				nullptr
			);
		}

		template <GPUCommandQueueType CmdListType>
		void GPUCommandContext<CmdListType>::CopyTextureToBuffer(const TextureCopyBufferSubAllocation& destSubAllocation, const TextureSubResource& srcTexture) const
		{
			assert(IsResourceAccessValid(destSubAllocation.GetBufferResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST) && "ERROR: The destination buffer resource in a call to GPUCommandContext::CopyTextureToBuffer() was not specified as a resource dependency with the D3D12_RESOURCE_STATE_COPY_DEST state!");
			assert(IsResourceAccessValid(srcTexture.GetGPUResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE) && "ERROR: The source texture resource in a call to GPUCommandContext::CopyTextureToBuffer() was not specified as a resource dependency with the D3D12_RESOURCE_STATE_COPY_SOURCE state!");

			const CD3DX12_TEXTURE_COPY_LOCATION destCopyLocation{ destSubAllocation.GetBufferTextureCopyLocation() };
			const CD3DX12_TEXTURE_COPY_LOCATION srcCopyLocation{ &(srcTexture.GetD3D12Resource()), srcTexture.GetSubResourceIndex() };

			mCmdList->CopyTextureRegion(
				&destCopyLocation,
				0,
				0,
				0,
				&srcCopyLocation,
				nullptr
			);
		}

		template <GPUCommandQueueType CmdListType>
		void GPUCommandContext<CmdListType>::CopyBufferToBuffer(const I_BufferSubAllocation& destSubAllocation, const I_BufferSubAllocation& srcSubAllocation) const
		{
			assert(IsResourceAccessValid(destSubAllocation.GetBufferResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST) && "ERROR: The destination buffer resource in a call to GPUCommandContext::CopyBufferToBuffer() was not specified as a resource dependency with the D3D12_RESOURCE_STATE_COPY_DEST state!");
			assert(IsResourceAccessValid(srcSubAllocation.GetBufferResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE) && "ERROR: The source buffer resource in a call to GPUCommandContext::CopyBufferToBuffer() was not specified as a resource dependency with the D3D12_RESOURCE_STATE_COPY_SOURCE state!");

			assert(destSubAllocation.GetSubAllocationSize() == srcSubAllocation.GetSubAllocationSize() && "ERROR: An attempt was made to call GPUCommandContext::CopyBufferToBuffer() with two buffer sub-allocations which did not have equivalent sizes!");
			
			mCmdList->CopyBufferRegion(
				&(destSubAllocation.GetD3D12Resource()),
				destSubAllocation.GetOffsetFromBufferStart(),
				&(srcSubAllocation.GetD3D12Resource()),
				srcSubAllocation.GetOffsetFromBufferStart(),
				srcSubAllocation.GetSubAllocationSize()
			);
		}
	}
}