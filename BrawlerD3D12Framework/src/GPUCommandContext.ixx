module;
#include <cassert>
#include <functional>
#include <array>
#include <span>
#include "DxDef.h"

export module Brawler.D3D12.GPUCommandContext;
import Brawler.D3D12.GPUCommandQueueType;
import Util.Engine;
import Util.General;
import Util.D3D12;
import Brawler.D3D12.GPUResourceAccessManager;
import Brawler.D3D12.FrameGraphResourceDependency;
import Brawler.D3D12.TextureCopyBufferSubAllocation;
import Brawler.D3D12.TextureSubResource;
import Brawler.D3D12.I_BufferSnapshot;
import Brawler.D3D12.BufferCopyRegion;
import Brawler.D3D12.TextureCopyRegion;
import Brawler.D3D12.GPUResourceDescriptorHeap;

namespace
{
	template <Brawler::D3D12::GPUCommandQueueType CmdListType>
	struct GPUCommandContextInfo
	{
		static_assert(sizeof(CmdListType) != sizeof(CmdListType), "ERROR: An explicit template specialization was not provided for GPUCommandContextInfo with a given Brawler::GPUCommandQueueType! (See GPUCommandContext.ixx.)");
	};

	template <D3D12_COMMAND_LIST_TYPE D3DCmdListType>
	struct GPUCommandContextInfoInstantiation
	{
		static constexpr D3D12_COMMAND_LIST_TYPE D3D_CMD_LIST_TYPE = D3DCmdListType;
	};

	template <>
	struct GPUCommandContextInfo<Brawler::D3D12::GPUCommandQueueType::DIRECT> : public GPUCommandContextInfoInstantiation<D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT>
	{};

	template <>
	struct GPUCommandContextInfo<Brawler::D3D12::GPUCommandQueueType::COMPUTE> : public GPUCommandContextInfoInstantiation<D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE>
	{};

	template <>
	struct GPUCommandContextInfo<Brawler::D3D12::GPUCommandQueueType::COPY> : public GPUCommandContextInfoInstantiation<D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY>
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

		template <typename T>
		class ComputeCapableCommandGenerator;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		template <typename DerivedType, GPUCommandQueueType CmdListType>
		class GPUCommandContext
		{
		private:
			friend GPUCommandQueue<CmdListType>;

			friend GPUCommandContextVault;

			template <GPUCommandQueueType QueueType>
			friend class GPUCommandListRecorder;

			template <GPUCommandQueueType CmdListType, typename InputDataType>
			friend class RenderPass;

			template <typename T>
			friend class ComputeCapableCommandGenerator;

		protected:
			GPUCommandContext();

		public:
			virtual ~GPUCommandContext() = default;

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

			void RecordCommandList(const std::function<void(DerivedType&)>& recordJob);

			/// <summary>
			/// Resets both the command list *AND* its underlying allocator. The latter action
			/// means that this function can only be called *AFTER* the GPU has finished
			/// executing all of the commands of this GPUCommandContext instance.
			/// </summary>
			void ResetCommandList();

			/// <summary>
			/// Records commands which must be recorded after the command list is reset, e.g.,
			/// setting the descriptor heaps. This function is called by 
			/// GPUCommandContext::ResetCommandList().
			/// </summary>
			void PrepareCommandList();

		private:
			void CloseCommandList();
			void MarkAsUseful();

			void ResourceBarrier(const std::span<const CD3DX12_RESOURCE_BARRIER> barrierSpan) const;

			// ==================================================================
			// ^ Implementation Details and General Functions | Shared Commands v
			// ==================================================================

		public:
			void AssertResourceState(const I_BufferSnapshot& bufferSnapshot, const D3D12_RESOURCE_STATES expectedState) const;
			void AssertResourceState(const TextureSubResource& textureSubResource, const D3D12_RESOURCE_STATES expectedState) const;

			void CopyBufferToTexture(const TextureSubResource& destTexture, const TextureCopyBufferSnapshot& srcSnapshot) const;
			void CopyTextureToBuffer(const TextureCopyBufferSnapshot& destSnapshot, const TextureSubResource& srcTexture) const;

			void CopyBufferToTexture(const TextureCopyRegion& destTexture, const TextureCopyBufferSnapshot& srcSnapshot) const;
			void CopyTextureToBuffer(const TextureCopyBufferSnapshot& destSnapshot, const TextureCopyRegion& srcTexture) const;

			void CopyBufferToBuffer(const I_BufferSnapshot& destSnapshot, const I_BufferSnapshot& srcSnapshot) const;
			void CopyBufferToBuffer(const BufferCopyRegion& destRegion, const BufferCopyRegion& srcRegion) const;

			void CopyTextureToTexture(const TextureSubResource& destTexture, const TextureSubResource& srcTexture) const;
			void CopyTextureToTexture(const TextureSubResource& destTexture, const TextureCopyRegion& srcRegion) const;
			void CopyTextureToTexture(const TextureCopyRegion& destRegion, const TextureSubResource& srcTexture) const;
			void CopyTextureToTexture(const TextureCopyRegion& destRegion, const TextureCopyRegion& srcRegion) const;

			/// <summary>
			/// Issues the D3D12 resource barrier specified by barrier immediately on the GPU timeline.
			/// 
			/// This function is meant for debugging the Brawler Engine's resource state tracking system. It
			/// should never be necessary to call this function for *ANY* barrier type if the system is working
			/// correctly, and this function does nothing in Release builds.
			/// </summary>
			/// <param name="barrier">
			/// - The CD3DX12_RESOURCE_BARRIER which is to be immediately issued on the GPU timeline.
			/// </param>
			void DebugResourceBarrier(const CD3DX12_RESOURCE_BARRIER& barrier) const;

		private:
			Microsoft::WRL::ComPtr<Brawler::D3D12GraphicsCommandList> mCmdList;
			GPUResourceAccessManager mResourceAccessManager;
			bool mHasCommands;
		};
	}
}

// ---------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <typename DerivedType, GPUCommandQueueType CmdListType>
		GPUCommandContext<DerivedType, CmdListType>::GPUCommandContext() :
			mCmdList(nullptr),
			mResourceAccessManager(),
			mHasCommands(false)
		{
			Brawler::D3D12Device& d3dDevice{ Util::Engine::GetD3D12Device() };

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

		template <typename DerivedType, GPUCommandQueueType CmdListType>
		Brawler::D3D12GraphicsCommandList& GPUCommandContext<DerivedType, CmdListType>::GetCommandList() const
		{
			return *(mCmdList.Get());
		}

		template <typename DerivedType, GPUCommandQueueType CmdListType>
		bool GPUCommandContext<DerivedType, CmdListType>::HasCommands() const
		{
			return mHasCommands;
		}

		template <typename DerivedType, GPUCommandQueueType CmdListType>
		bool GPUCommandContext<DerivedType, CmdListType>::IsResourceAccessValid(const I_GPUResource& resource, const D3D12_RESOURCE_STATES requiredState) const
		{
			if constexpr (Util::General::IsDebugModeEnabled())
				return mResourceAccessManager.IsResourceAccessValid(resource, requiredState);
			else
				return true;
		}

		template <typename DerivedType, GPUCommandQueueType CmdListType>
		void GPUCommandContext<DerivedType, CmdListType>::SetValidGPUResources(const std::span<const FrameGraphResourceDependency> dependencySpan)
		{
			mResourceAccessManager.SetCurrentResourceDependencies(dependencySpan);
		}

		template <typename DerivedType, GPUCommandQueueType CmdListType>
		void GPUCommandContext<DerivedType, CmdListType>::RecordCommandList(const std::function<void(DerivedType&)>& recordJob)
		{
			// Record the commands into the command list.
			static_cast<DerivedType*>(this)->RecordCommandListIMPL(recordJob);
		}

		template <typename DerivedType, GPUCommandQueueType CmdListType>
		void GPUCommandContext<DerivedType, CmdListType>::ResetCommandList()
		{
			// Reset the command list so that we can record commands into it.
			Util::General::CheckHRESULT(mCmdList->Reset(&(Util::Engine::GetD3D12CommandAllocator(CmdListType)), nullptr));

			PrepareCommandList();

			// At the beginning of a command list, it has no commands recorded into it.
			mHasCommands = false;
		}

		template <typename DerivedType, GPUCommandQueueType CmdListType>
		void GPUCommandContext<DerivedType, CmdListType>::PrepareCommandList()
		{
			// For command lists which will be sent to either the direct or the compute
			// queue, we should set the descriptor heaps before recording any commands.
			if constexpr (CmdListType != GPUCommandQueueType::COPY)
			{
				static const std::array<Brawler::D3D12DescriptorHeap*, 1> shaderVisibleDescriptorHeapArr{
					&(Util::Engine::GetGPUResourceDescriptorHeap().GetD3D12DescriptorHeap())
				};

				mCmdList->SetDescriptorHeaps(static_cast<std::uint32_t>(shaderVisibleDescriptorHeapArr.size()), shaderVisibleDescriptorHeapArr.data());
			}

			static_cast<DerivedType*>(this)->PrepareCommandListIMPL();
		}

		template <typename DerivedType, GPUCommandQueueType CmdListType>
		void GPUCommandContext<DerivedType, CmdListType>::CloseCommandList()
		{
			// Close the command list. This lets D3D12 know that we are finished recording commands
			// into it.
			Util::General::CheckHRESULT(mCmdList->Close());
		}

		template <typename DerivedType, GPUCommandQueueType CmdListType>
		void GPUCommandContext<DerivedType, CmdListType>::MarkAsUseful()
		{
			mHasCommands = true;
		}

		template <typename DerivedType, GPUCommandQueueType CmdListType>
		void GPUCommandContext<DerivedType, CmdListType>::ResourceBarrier(const std::span<const CD3DX12_RESOURCE_BARRIER> barrierSpan) const
		{
			if (!barrierSpan.empty())
				mCmdList->ResourceBarrier(static_cast<std::uint32_t>(barrierSpan.size()), barrierSpan.data());
		}

		// ==================================================================
		// ^ Implementation Details and General Functions | Shared Commands v
		// ==================================================================

		template <typename DerivedType, GPUCommandQueueType CmdListType>
		void GPUCommandContext<DerivedType, CmdListType>::AssertResourceState(const I_BufferSnapshot& bufferSnapshot, const D3D12_RESOURCE_STATES expectedState) const
		{
			if (Util::D3D12::IsDebugLayerEnabled())
			{
				assert(Util::D3D12::IsResourceStateValid(expectedState) && "ERROR: An attempt was made to check if a resource is in a given state using GPUCommandContext::AssertResourceState(), but the provided resource state was invalid!");

				Microsoft::WRL::ComPtr<Brawler::D3D12DebugCommandList> debugCmdList{};
				Util::General::CheckHRESULT(mCmdList.As(&debugCmdList));

				assert(debugCmdList->AssertResourceState(&(bufferSnapshot.GetD3D12Resource()), 0, static_cast<std::uint32_t>(expectedState)) && "ERROR: A D3D12 resource state assertion failed! (See GPUCommandContext::AssertResourceState().)");
			}
		}

		template <typename DerivedType, GPUCommandQueueType CmdListType>
		void GPUCommandContext<DerivedType, CmdListType>::AssertResourceState(const TextureSubResource& textureSubResource, const D3D12_RESOURCE_STATES expectedState) const
		{
			if (Util::D3D12::IsDebugLayerEnabled())
			{
				assert(Util::D3D12::IsResourceStateValid(expectedState) && "ERROR: An attempt was made to check if a resource is in a given state using GPUCommandContext::AssertResourceState(), but the provided resource state was invalid!");

				Microsoft::WRL::ComPtr<Brawler::D3D12DebugCommandList> debugCmdList{};
				Util::General::CheckHRESULT(mCmdList.As(&debugCmdList));

				assert(debugCmdList->AssertResourceState(&(textureSubResource.GetD3D12Resource()), textureSubResource.GetSubResourceIndex(), static_cast<std::uint32_t>(expectedState)) && "ERROR: A D3D12 resource state assertion failed! (See GPUCommandContext::AssertResourceState().)");
			}
		}

		template <typename DerivedType, GPUCommandQueueType CmdListType>
		void GPUCommandContext<DerivedType, CmdListType>::CopyBufferToTexture(const TextureSubResource& destTexture, const TextureCopyBufferSnapshot& srcSnapshot) const
		{
			assert(IsResourceAccessValid(destTexture.GetGPUResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST) && "ERROR: The destination texture resource in a call to GPUCommandContext::CopyBufferToTexture() was not specified as a resource dependency with the D3D12_RESOURCE_STATE_COPY_DEST state!");
			assert(IsResourceAccessValid(srcSnapshot.GetBufferResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE) && "ERROR: The source buffer resource in a call to GPUCommandContext::CopyBufferToTexture() was not specified as a resource dependency with the D3D12_RESOURCE_STATE_COPY_SOURCE state!");

			const CD3DX12_TEXTURE_COPY_LOCATION destCopyLocation{ &(destTexture.GetD3D12Resource()), destTexture.GetSubResourceIndex() };
			const CD3DX12_TEXTURE_COPY_LOCATION& srcCopyLocation{ srcSnapshot.GetBufferTextureCopyLocation() };

			mCmdList->CopyTextureRegion(
				&destCopyLocation,
				0,
				0,
				0,
				&srcCopyLocation,
				nullptr
			);
		}

		template <typename DerivedType, GPUCommandQueueType CmdListType>
		void GPUCommandContext<DerivedType, CmdListType>::CopyTextureToBuffer(const TextureCopyBufferSnapshot& destSnapshot, const TextureSubResource& srcTexture) const
		{
			assert(IsResourceAccessValid(destSnapshot.GetBufferResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST) && "ERROR: The destination buffer resource in a call to GPUCommandContext::CopyTextureToBuffer() was not specified as a resource dependency with the D3D12_RESOURCE_STATE_COPY_DEST state!");
			assert(IsResourceAccessValid(srcTexture.GetGPUResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE) && "ERROR: The source texture resource in a call to GPUCommandContext::CopyTextureToBuffer() was not specified as a resource dependency with the D3D12_RESOURCE_STATE_COPY_SOURCE state!");

			const CD3DX12_TEXTURE_COPY_LOCATION& destCopyLocation{ destSnapshot.GetBufferTextureCopyLocation() };
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

		template <typename DerivedType, GPUCommandQueueType CmdListType>
		void GPUCommandContext<DerivedType, CmdListType>::CopyBufferToTexture(const TextureCopyRegion& destTexture, const TextureCopyBufferSnapshot& srcSnapshot) const
		{
			assert(IsResourceAccessValid(destTexture.GetGPUResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST) && "ERROR: The destination texture resource in a call to GPUCommandContext::CopyBufferToTexture() was not specified as a resource dependency with the D3D12_RESOURCE_STATE_COPY_DEST state!");
			assert(IsResourceAccessValid(srcSnapshot.GetBufferResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE) && "ERROR: The source buffer resource in a call to GPUCommandContext::CopyBufferToTexture() was not specified as a resource dependency with the D3D12_RESOURCE_STATE_COPY_SOURCE state!");

			const CD3DX12_TEXTURE_COPY_LOCATION destCopyLocation{ destTexture.GetTextureCopyLocation() };
			const CD3DX12_TEXTURE_COPY_LOCATION& srcCopyLocation{ srcSnapshot.GetBufferTextureCopyLocation() };

			const CD3DX12_BOX& destCopyRegionBox{ destTexture.GetCopyRegionBox() };

			mCmdList->CopyTextureRegion(
				&destCopyLocation,
				destCopyRegionBox.left,
				destCopyRegionBox.top,
				destCopyRegionBox.front,
				&srcCopyLocation,
				nullptr
			);
		}

		template <typename DerivedType, GPUCommandQueueType CmdListType>
		void GPUCommandContext<DerivedType, CmdListType>::CopyTextureToBuffer(const TextureCopyBufferSnapshot& destSnapshot, const TextureCopyRegion& srcTexture) const
		{
			assert(IsResourceAccessValid(destSnapshot.GetBufferResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST) && "ERROR: The destination buffer resource in a call to GPUCommandContext::CopyTextureToBuffer() was not specified as a resource dependency with the D3D12_RESOURCE_STATE_COPY_DEST state!");
			assert(IsResourceAccessValid(srcTexture.GetGPUResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE) && "ERROR: The source texture resource in a call to GPUCommandContext::CopyTextureToBuffer() was not specified as a resource dependency with the D3D12_RESOURCE_STATE_COPY_SOURCE state!");

			const CD3DX12_TEXTURE_COPY_LOCATION& destCopyLocation{ destSnapshot.GetBufferTextureCopyLocation() };
			const CD3DX12_TEXTURE_COPY_LOCATION srcCopyLocation{ srcTexture.GetTextureCopyLocation() };

			const CD3DX12_BOX& srcCopyRegionBox{ srcTexture.GetCopyRegionBox() };

			mCmdList->CopyTextureRegion(
				&destCopyLocation,
				0,
				0,
				0,
				&srcCopyLocation,
				&srcCopyRegionBox
			);
		}

		template <typename DerivedType, GPUCommandQueueType CmdListType>
		void GPUCommandContext<DerivedType, CmdListType>::CopyBufferToBuffer(const I_BufferSnapshot& destSnapshot, const I_BufferSnapshot& srcSnapshot) const
		{
			assert(IsResourceAccessValid(destSnapshot.GetBufferResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST) && "ERROR: The destination buffer resource in a call to GPUCommandContext::CopyBufferToBuffer() was not specified as a resource dependency with the D3D12_RESOURCE_STATE_COPY_DEST state!");
			assert(IsResourceAccessValid(srcSnapshot.GetBufferResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE) && "ERROR: The source buffer resource in a call to GPUCommandContext::CopyBufferToBuffer() was not specified as a resource dependency with the D3D12_RESOURCE_STATE_COPY_SOURCE state!");

			assert(destSnapshot.GetSubAllocationSize() == srcSnapshot.GetSubAllocationSize() && "ERROR: An attempt was made to call GPUCommandContext::CopyBufferToBuffer() with two buffer sub-allocations which did not have equivalent sizes!");
			
			mCmdList->CopyBufferRegion(
				&(destSnapshot.GetD3D12Resource()),
				destSnapshot.GetOffsetFromBufferStart(),
				&(srcSnapshot.GetD3D12Resource()),
				srcSnapshot.GetOffsetFromBufferStart(),
				srcSnapshot.GetSubAllocationSize()
			);
		}

		template <typename DerivedType, GPUCommandQueueType CmdListType>
		void GPUCommandContext<DerivedType, CmdListType>::CopyBufferToBuffer(const BufferCopyRegion& destRegion, const BufferCopyRegion& srcRegion) const
		{
			assert(IsResourceAccessValid(destRegion.GetBufferResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST) && "ERROR: The destination buffer resource in a call to GPUCommandContext::CopyBufferToBuffer() was not specified as a resource dependency with the D3D12_RESOURCE_STATE_COPY_DEST state!");
			assert(IsResourceAccessValid(srcRegion.GetBufferResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE) && "ERROR: The source buffer resource in a call to GPUCommandContext::CopyBufferToBuffer() was not specified as a resource dependency with the D3D12_RESOURCE_STATE_COPY_SOURCE state!");

			assert(destRegion.GetCopyRegionSize() >= srcRegion.GetCopyRegionSize() && "ERROR: An attempt was made to call GPUCommandContext::CopyBufferToBuffer() with two buffer copy regions, but the destination region was not large enough to hold all of the data from the source region!");

			mCmdList->CopyBufferRegion(
				&(destRegion.GetD3D12Resource()),
				destRegion.GetOffsetFromBufferStart(),
				&(srcRegion.GetD3D12Resource()),
				srcRegion.GetOffsetFromBufferStart(),
				srcRegion.GetCopyRegionSize()
			);
		}

		template <typename DerivedType, GPUCommandQueueType CmdListType>
		void GPUCommandContext<DerivedType, CmdListType>::CopyTextureToTexture(const TextureSubResource& destTexture, const TextureSubResource& srcTexture) const
		{
			assert(IsResourceAccessValid(destTexture.GetGPUResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST) && "ERROR: The destination texture resource in a call to GPUCommandContext::CopyTextureToTexture() was not specified as a resource dependency with the D3D12_RESOURCE_STATE_COPY_DEST state!");
			assert(IsResourceAccessValid(srcTexture.GetGPUResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE) && "ERROR: The source texture resource in a call to GPUCommandContext::CopyTextureToTexture() was not specified as a resource dependency with the D3D12_RESOURCE_STATE_COPY_SOURCE state!");

			assert(((&(destTexture.GetGPUResource()) != &(srcTexture.GetGPUResource())) || (destTexture.GetSubResourceIndex() != srcTexture.GetSubResourceIndex())) && "ERROR: An attempt was made to call GPUCommandContext::CopyTextureToTexture() with the exact same sub-resource being specified for both the source and the destination!");
			
			// According to the MSDN, it is best to use ID3D12GraphicsCommandList::CopyResource() to copy an
			// entire resource, rather than use ID3D12GraphicsCommandList::CopyTextureRegion().

			if (destTexture.GetGPUResource().GetSubResourceCount() == 1 && srcTexture.GetGPUResource().GetSubResourceCount() == 1) [[unlikely]]
				mCmdList->CopyResource(&(destTexture.GetD3D12Resource()), &(srcTexture.GetD3D12Resource()));
			else [[likely]]
			{
				const CD3DX12_TEXTURE_COPY_LOCATION destinationCopyLocation{ &(destTexture.GetD3D12Resource()), destTexture.GetSubResourceIndex() };
				const CD3DX12_TEXTURE_COPY_LOCATION srcCopyLocation{ &(srcTexture.GetD3D12Resource()), srcTexture.GetSubResourceIndex() };

				mCmdList->CopyTextureRegion(
					&destinationCopyLocation,
					0,
					0,
					0,
					&srcCopyLocation,
					nullptr
				);
			}
		}

		template <typename DerivedType, GPUCommandQueueType CmdListType>
		void GPUCommandContext<DerivedType, CmdListType>::CopyTextureToTexture(const TextureSubResource& destTexture, const TextureCopyRegion& srcRegion) const
		{
			assert(IsResourceAccessValid(destTexture.GetGPUResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST) && "ERROR: The destination texture resource in a call to GPUCommandContext::CopyTextureToTexture() was not specified as a resource dependency with the D3D12_RESOURCE_STATE_COPY_DEST state!");
			assert(IsResourceAccessValid(srcRegion.GetGPUResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE) && "ERROR: The source texture resource in a call to GPUCommandContext::CopyTextureToTexture() was not specified as a resource dependency with the D3D12_RESOURCE_STATE_COPY_SOURCE state!");

			assert(((&(destTexture.GetGPUResource()) != &(srcRegion.GetGPUResource())) || (destTexture.GetSubResourceIndex() != srcRegion.GetSubResourceIndex())) && "ERROR: An attempt was made to call GPUCommandContext::CopyTextureToTexture() with the exact same sub-resource being specified for both the source and the destination!");

			const CD3DX12_TEXTURE_COPY_LOCATION destinationCopyLocation{ &(destTexture.GetD3D12Resource()), destTexture.GetSubResourceIndex() };

			const CD3DX12_TEXTURE_COPY_LOCATION srcCopyLocation{ srcRegion.GetTextureCopyLocation() };
			const CD3DX12_BOX& srcCopyRegionBox{ srcRegion.GetCopyRegionBox() };

			mCmdList->CopyTextureRegion(
				&destinationCopyLocation,
				0,
				0,
				0,
				&srcCopyLocation,
				&srcCopyRegionBox
			);
		}

		template <typename DerivedType, GPUCommandQueueType CmdListType>
		void GPUCommandContext<DerivedType, CmdListType>::CopyTextureToTexture(const TextureCopyRegion& destRegion, const TextureSubResource& srcTexture) const
		{
			assert(IsResourceAccessValid(destRegion.GetGPUResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST) && "ERROR: The destination texture resource in a call to GPUCommandContext::CopyTextureToTexture() was not specified as a resource dependency with the D3D12_RESOURCE_STATE_COPY_DEST state!");
			assert(IsResourceAccessValid(srcTexture.GetGPUResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE) && "ERROR: The source texture resource in a call to GPUCommandContext::CopyTextureToTexture() was not specified as a resource dependency with the D3D12_RESOURCE_STATE_COPY_SOURCE state!");

			assert(((&(destRegion.GetGPUResource()) != &(srcTexture.GetGPUResource())) || (destRegion.GetSubResourceIndex() != srcTexture.GetSubResourceIndex())) && "ERROR: An attempt was made to call GPUCommandContext::CopyTextureToTexture() with the exact same sub-resource being specified for both the source and the destination!");

			const CD3DX12_TEXTURE_COPY_LOCATION destinationCopyLocation{ destRegion.GetTextureCopyLocation() };
			const CD3DX12_BOX& destinationCopyRegionBox{ destRegion.GetCopyRegionBox() };

			const CD3DX12_TEXTURE_COPY_LOCATION srcCopyLocation{ &(srcTexture.GetD3D12Resource()), srcTexture.GetSubResourceIndex() };

			mCmdList->CopyTextureRegion(
				&destinationCopyLocation,
				destinationCopyRegionBox.left,
				destinationCopyRegionBox.top,
				destinationCopyRegionBox.front,
				&srcCopyLocation,
				nullptr
			);
		}

		template <typename DerivedType, GPUCommandQueueType CmdListType>
		void GPUCommandContext<DerivedType, CmdListType>::CopyTextureToTexture(const TextureCopyRegion& destRegion, const TextureCopyRegion& srcRegion) const
		{
			assert(IsResourceAccessValid(destRegion.GetGPUResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST) && "ERROR: The destination texture resource in a call to GPUCommandContext::CopyTextureToTexture() was not specified as a resource dependency with the D3D12_RESOURCE_STATE_COPY_DEST state!");
			assert(IsResourceAccessValid(srcRegion.GetGPUResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE) && "ERROR: The source texture resource in a call to GPUCommandContext::CopyTextureToTexture() was not specified as a resource dependency with the D3D12_RESOURCE_STATE_COPY_SOURCE state!");

			assert(((&(destRegion.GetGPUResource()) != &(srcRegion.GetGPUResource())) || (destRegion.GetSubResourceIndex() != srcRegion.GetSubResourceIndex())) && "ERROR: An attempt was made to call GPUCommandContext::CopyTextureToTexture() with the exact same sub-resource being specified for both the source and the destination!");

			const CD3DX12_TEXTURE_COPY_LOCATION destinationCopyLocation{ destRegion.GetTextureCopyLocation() };
			const CD3DX12_BOX& destinationCopyRegionBox{ destRegion.GetCopyRegionBox() };

			const CD3DX12_TEXTURE_COPY_LOCATION srcCopyLocation{ srcRegion.GetTextureCopyLocation() };
			const CD3DX12_BOX& srcCopyRegionBox{ srcRegion.GetCopyRegionBox() };

			mCmdList->CopyTextureRegion(
				&destinationCopyLocation,
				destinationCopyRegionBox.left,
				destinationCopyRegionBox.top,
				destinationCopyRegionBox.front,
				&srcCopyLocation,
				&srcCopyRegionBox
			);
		}

		template <typename DerivedType, GPUCommandQueueType CmdListType>
		void GPUCommandContext<DerivedType, CmdListType>::DebugResourceBarrier(const CD3DX12_RESOURCE_BARRIER& barrier) const
		{
			if constexpr (Util::General::IsDebugModeEnabled())
				mCmdList->ResourceBarrier(1, &barrier);
		}
	}
}