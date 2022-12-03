module;
#include <optional>
#include <cassert>
#include <shared_mutex>
#include <memory>
#include "DxDef.h"

module Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.GPUResourceInitializationInfo;
import Util.Engine;
import Util.Math;
import Brawler.ScopedSharedLock;
import Brawler.D3D12.CommittedD3D12ResourceContainer;
import Brawler.D3D12.PlacedD3D12ResourceContainer;
import Brawler.D3D12.BorrowedD3D12ResourceContainer;
import Brawler.OptionalRef;

namespace Brawler
{
	namespace D3D12
	{
		I_GPUResource::I_GPUResource(GPUResourceInitializationInfo&& initInfo) :
			mResourceContainer(nullptr),
			mLifetimeType(GPUResourceLifetimeType::PERSISTENT),
			mInitInfo(std::move(initInfo)),
			mUsageTracker(),
			mStateManager(initInfo.InitialResourceState, GetSubResourceCount()),
			mRequiresSpecialInitialization((mInitInfo.ResourceDesc.Flags & (D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) != 0),
			mBindlessSRVManager(),
			mResourceCritSection()
		{
#ifdef _DEBUG
			// Only buffers can be located in UPLOAD or READBACK heaps.
			if (mInitInfo.HeapType != D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT)
				assert(mInitInfo.ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER && "ERROR: Only buffers can be located in a D3D12_HEAP_TYPE_UPLOAD or D3D12_HEAP_TYPE_READBACK heap!");

			// Resources created in UPLOAD heaps must start in and cannot (explicitly) transition from the
			// GENERIC_READ state.
			if (mInitInfo.HeapType == D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD)
				assert(mInitInfo.InitialResourceState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ && "ERROR: Resources created in a D3D12_HEAP_TYPE_UPLOAD heap *MUST* start in the D3D12_RESOURCE_STATE_GENERIC_READ state, and cannot (explicitly) transition out of that state!");

			// Resources created in READBACK heaps must start in and cannot (explicitly) transition from the
			// COPY_DEST state.
			if (mInitInfo.HeapType == D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK)
				assert(mInitInfo.InitialResourceState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST && "ERROR: Resources created in a D3D12_HEAP_TYPE_READBACK heap *MUST* start in the D3D12_RESOURCE_STATE_COPY_DEST state, and cannot (explicitly) transition out of that state!");

			// Render targets must start in the RENDER_TARGET state. Similarly, depth/stencil textures must
			// start in the DEPTH_WRITE state. This is because of the requirement to initialize these textures
			// as their first rendering operation, and not because the D3D12 API requires it (which it does not,
			// by the way).
			if ((mInitInfo.ResourceDesc.Flags & D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0)
				assert(mInitInfo.InitialResourceState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET && "ERROR: Render target textures *MUST* start in the D3D12_RESOURCE_STATE_RENDER_TARGET state!");

			if ((mInitInfo.ResourceDesc.Flags & D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0)
				assert(mInitInfo.InitialResourceState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE && "ERROR: Depth/stencil textures *MUST* start in the D3D12_RESOURCE_STATE_DEPTH_WRITE state!");

			// It is not valid for a texture to be both a render target and a depth/stencil texture.
			assert(Util::Math::CountOneBits(std::to_underlying(mInitInfo.ResourceDesc.Flags & (D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL))) <= 1 && "ERROR: It is not valid for a resource to be both a render target and a depth/stencil texture!");

			// Buffers and simultaneous-access textures created in a DEFAULT heap are implicitly promoted on
			// their first use on the GPU. This implies that they actually start in the COMMON state in this
			// case. (This doesn't seem to be mentioned anywhere on the MSDN, of course. PIX points this
			// out as a warning.)
			{
				const bool implicitlyPromotedOnFirstUse = (mInitInfo.ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER || (mInitInfo.ResourceDesc.Flags & D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS) != 0);

				if (implicitlyPromotedOnFirstUse && mInitInfo.HeapType == D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT)
					assert(mInitInfo.InitialResourceState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON && "ERROR: Buffers and simultaneous-access textures are implicitly promoted on their first use on the GPU during a call to ExecuteCommandLists() (including the first call to this function, before the resource has ever been used). Thus, to ensure optimal barrier creation, resources of either of these types should start in the D3D12_RESOURCE_STATE_COMMON state!");
			}
#endif // _DEBUG
		}

		std::uint32_t I_GPUResource::GetSubResourceCount() const
		{
			const Brawler::D3D12_RESOURCE_DESC& resourceDesc{ GetResourceDescription() };
			
			// Buffers only ever have one subresource.
			if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER)
				return 1;

			// Otherwise, the D3D12CalcSubresource() function (provided with d3dx12.h) provides
			// a means to get the *INDEX* of a particular subresource.
			const std::uint32_t maxIndex = D3D12CalcSubresource(
				(static_cast<std::uint32_t>(resourceDesc.MipLevels) - 1),
				(static_cast<std::uint32_t>(resourceDesc.DepthOrArraySize) - 1),
				(static_cast<std::uint32_t>(D3D12GetFormatPlaneCount(&(Util::Engine::GetD3D12Device()), resourceDesc.Format)) - 1),
				resourceDesc.MipLevels,
				resourceDesc.DepthOrArraySize
			);

			return (maxIndex + 1);
		}

		std::optional<D3D12_CLEAR_VALUE> I_GPUResource::GetOptimizedClearValue() const
		{
			return std::optional<D3D12_CLEAR_VALUE>{};
		}

		GPUResourceCreationType I_GPUResource::GetPreferredCreationType() const
		{
			return GPUResourceCreationType::PLACED;
		}

		void I_GPUResource::CreateCommittedD3D12Resource()
		{
			std::unique_ptr<CommittedD3D12ResourceContainer> committedResourceContainer{ std::make_unique<CommittedD3D12ResourceContainer>(*this) };

			{
				Brawler::ScopedSharedWriteLock<std::shared_mutex> writeLock{ mResourceCritSection };

				committedResourceContainer->CreateCommittedD3D12Resource(GetCommittedResourceHeapFlags(), mInitInfo);

				// When we assign an I_GPUResource instance a new ID3D12Resource*, all of its
				// previous descriptors become invalidated. This is because it now has a new
				// GPU virtual address.
				UpdateDescriptors(committedResourceContainer->GetD3D12Resource());

				mResourceContainer = std::move(committedResourceContainer);
			}

			ExecutePostD3D12ResourceInitializationCallback();
		}

		void I_GPUResource::CreatePlacedD3D12Resource(GPUResourceAllocationHandle&& hAllocation)
		{
			std::unique_ptr<PlacedD3D12ResourceContainer> placedResourceContainer{ std::make_unique<PlacedD3D12ResourceContainer>(*this) };
			
			{
				Brawler::ScopedSharedWriteLock<std::shared_mutex> writeLock{ mResourceCritSection };

				placedResourceContainer->CreatePlacedD3D12Resource(std::move(hAllocation), mInitInfo);

				// When we assign an I_GPUResource instance a new ID3D12Resource*, all of its
				// previous descriptors become invalidated. This is because it now has a new
				// GPU virtual address.
				UpdateDescriptors(placedResourceContainer->GetD3D12Resource());

				mResourceContainer = std::move(placedResourceContainer);
			}
			
			ExecutePostD3D12ResourceInitializationCallback();
		}

		void I_GPUResource::BorrowD3D12Resource(Microsoft::WRL::ComPtr<Brawler::D3D12Resource>&& d3dResourcePtr)
		{
			std::unique_ptr<BorrowedD3D12ResourceContainer> borrowedResourceContainer{ std::make_unique<BorrowedD3D12ResourceContainer>(*this) };

			{
				Brawler::ScopedSharedWriteLock<std::shared_mutex> writeLock{ mResourceCritSection };

				borrowedResourceContainer->BorrowD3D12Resource(std::move(d3dResourcePtr));

				// When we assign an I_GPUResource instance a new ID3D12Resource*, all of its
				// previous descriptors become invalidated. This is because it now has a new
				// GPU virtual address.
				UpdateDescriptors(borrowedResourceContainer->GetD3D12Resource());

				mResourceContainer = std::move(borrowedResourceContainer);
			}

			ExecutePostD3D12ResourceInitializationCallback();
		}

		BindlessSRVAllocation I_GPUResource::CreateBindlessSRV(D3D12_SHADER_RESOURCE_VIEW_DESC&& srvDesc)
		{
			Brawler::ScopedSharedReadLock<std::shared_mutex> readLock{ mResourceCritSection };

			if (mResourceContainer != nullptr)
				return mBindlessSRVManager.CreateBindlessSRV(std::move(srvDesc), Brawler::OptionalRef<Brawler::D3D12Resource>{mResourceContainer->GetD3D12Resource()});

			return mBindlessSRVManager.CreateBindlessSRV(std::move(srvDesc), Brawler::OptionalRef<Brawler::D3D12Resource>{});
		}

		bool I_GPUResource::HasBindlessSRVs() const
		{
			return mBindlessSRVManager.HasBindlessSRVs();
		}

		void I_GPUResource::ExecutePostD3D12ResourceInitializationCallback()
		{}

		D3D12_HEAP_FLAGS I_GPUResource::GetCommittedResourceHeapFlags() const
		{
			return D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE;
		}

		Brawler::D3D12Resource& I_GPUResource::GetD3D12Resource() const
		{
			Brawler::ScopedSharedReadLock<std::shared_mutex> readLock{ mResourceCritSection };

			assert(mResourceContainer != nullptr && "ERROR: An attempt was made to access the ID3D12Resource instance of an I_GPUResource before it could be initialized!");
			return mResourceContainer->GetD3D12Resource();
		}

		bool I_GPUResource::IsD3D12ResourceCreated() const
		{
			Brawler::ScopedSharedReadLock<std::shared_mutex> readLock{ mResourceCritSection };

			return (mResourceContainer != nullptr);
		}

		bool I_GPUResource::CanAliasBeforeUseOnGPU() const
		{
			return true;
		}

		bool I_GPUResource::CanAliasAfterUseOnGPU() const
		{
			return true;
		}

		D3D12_RESOURCE_STATES I_GPUResource::GetSubResourceState(const std::uint32_t subResourceIndex) const
		{
			return mStateManager.GetSubResourceState(subResourceIndex);
		}

		std::span<const D3D12_RESOURCE_STATES> I_GPUResource::GetAllSubResourceStates() const
		{
			return mStateManager.GetAllSubResourceStates();
		}

		void I_GPUResource::SetSubResourceState(const D3D12_RESOURCE_STATES newState, const std::uint32_t subResourceIndex)
		{
			// Resources in UPLOAD or READBACK heaps can never (explicitly?) transition out of
			// the state which they start in.
			assert(mInitInfo.HeapType == D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT && "ERROR: Resources created in either D3D12_HEAP_TYPE_UPLOAD or D3D12_HEAP_TYPE_READBACK heaps can never (explicitly) transition out of their initial state!");

			mStateManager.SetSubResourceState(newState, subResourceIndex);
		}

		const Brawler::D3D12_RESOURCE_DESC& I_GPUResource::GetResourceDescription() const
		{
			return mInitInfo.ResourceDesc;
		}

		void I_GPUResource::SetResourceDescription(Brawler::D3D12_RESOURCE_DESC&& resourceDesc)
		{
			Brawler::ScopedSharedReadLock<std::shared_mutex> readLock{ mResourceCritSection };
			assert(mResourceContainer == nullptr && "ERROR: An attempt was made to change the description of a resource after it was already created!");
			
			mInitInfo.ResourceDesc = std::move(resourceDesc);
		}

		D3D12_HEAP_TYPE I_GPUResource::GetHeapType() const
		{
			return mInitInfo.HeapType;
		}

		GPUResourceLifetimeType I_GPUResource::GetGPUResourceLifetimeType() const
		{
			return mLifetimeType;
		}

		const GPUResourceUsageTracker& I_GPUResource::GetUsageTracker() const
		{
			return mUsageTracker;
		}

		void I_GPUResource::SetGPUResourceLifetimeType(const GPUResourceLifetimeType lifetimeType)
		{
			mLifetimeType = lifetimeType;
		}

		void I_GPUResource::MarkAsUsedForCurrentFrame()
		{
			mUsageTracker.MarkAsUsedForCurrentFrame();
		}

		void I_GPUResource::UpdateDescriptors(Brawler::D3D12Resource& newD3DResource)
		{
			// It shouldn't be necessary to invalidate per-frame descriptor tables. We only destroy
			// resources and their ID3D12Resource objects if we can guarantee that the GPU is no
			// longer using them.

			mBindlessSRVManager.UpdateBindlessSRVs(newD3DResource);
		}

		GPUResourceSpecialInitializationMethod I_GPUResource::GetPreferredSpecialInitializationMethod() const
		{
			return GPUResourceSpecialInitializationMethod::DISCARD;
		}

		bool I_GPUResource::RequiresSpecialInitialization() const
		{
			return mRequiresSpecialInitialization;
		}

		void I_GPUResource::MarkSpecialInitializationAsCompleted()
		{
			mRequiresSpecialInitialization = false;
		}
	}
}