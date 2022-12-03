module;
#include <optional>
#include <shared_mutex>
#include <memory>
#include <span>
#include "DxDef.h"

export module Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.GPUResourceSpecialInitializationMethod;
import Brawler.D3D12.GPUResourceInitializationInfo;
import Brawler.D3D12.GPUResourceLifetimeType;
import Brawler.D3D12.GPUResourceAllocationHandle;
import Brawler.D3D12.GPUResourceUsageTracker;
import Brawler.D3D12.D3D12ResourceContainer;
import Brawler.D3D12.GPUResourceCreationType;
import Brawler.D3D12.BindlessSRVAllocation;
import Brawler.D3D12.GPUSubResourceStateManager;

namespace Brawler
{
	namespace D3D12
	{
		class GPUResourceStateTracker;
		class GPUSubResourceStateBarrierMerger;
		class FrameGraphBuilder;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class I_GPUResource
		{
		private:
			friend class GPUResourceStateTracker;
			friend class GPUSubResourceStateBarrierMerger;
			friend class FrameGraphBuilder;

		protected:
			explicit I_GPUResource(GPUResourceInitializationInfo&& initInfo);

		public:
			virtual ~I_GPUResource() = default;

			I_GPUResource(const I_GPUResource& rhs) = delete;
			I_GPUResource& operator=(const I_GPUResource& rhs) = delete;

			I_GPUResource(I_GPUResource&& rhs) noexcept = default;
			I_GPUResource& operator=(I_GPUResource&& rhs) noexcept = default;

			/// <summary>
			/// Returns the number of D3D12 subresources which comprise this I_GPUResource
			/// instance.
			/// </summary>
			/// <returns>
			/// The function returns the number of D3D12 subresources which comprise this 
			/// I_GPUResource instance.
			/// </returns>
			std::uint32_t GetSubResourceCount() const;

			virtual std::optional<D3D12_CLEAR_VALUE> GetOptimizedClearValue() const;

			virtual GPUResourceCreationType GetPreferredCreationType() const;

			void CreateCommittedD3D12Resource();
			void CreatePlacedD3D12Resource(GPUResourceAllocationHandle&& hAllocation);

			/// <summary>
			/// Assigns this I_GPUResource instance an ID3D12Resource which is owned by some other
			/// component of the application. It is not expected that this I_GPUResource instance
			/// manage the lifetime of the ID3D12Resource instance assigned to it when using
			/// I_GPUResource::BorrowD3D12Resource() (hence the term "borrow").
			/// 
			/// There are VERY few use cases for this, with the most notable one being obtaining
			/// a reference to the ID3D12Resource objects behind an IDXGISwapChain instance's
			/// back buffers. This function should only really be used when the application has
			/// no control over how the ID3D12Resource object in question is to be created.
			/// </summary>
			/// <param name="d3dResourcePtr">
			/// - A Microsoft::WRL::ComPtr<Brawler::D3D12Resource> which refers to the ID3D12Resource
			///   instance being borrowed.
			/// </param>
			void BorrowD3D12Resource(Microsoft::WRL::ComPtr<Brawler::D3D12Resource>&& d3dResourcePtr);

			BindlessSRVAllocation CreateBindlessSRV(D3D12_SHADER_RESOURCE_VIEW_DESC&& srvDesc);
			bool HasBindlessSRVs() const;

		protected:
			/// <summary>
			/// This virtual function is called immediately after the ID3D12Resource of this
			/// I_GPUResource is initialized.
			/// 
			/// Derived classes can override this function to execute custom tasks as a
			/// result of the resource's creation.
			/// </summary>
			virtual void ExecutePostD3D12ResourceInitializationCallback();

			/// <summary>
			/// When creating a committed resource, there are certain D3D12_HEAP_FLAGS values
			/// which can be used. Many actually have no meaning for committed heaps, but some
			/// are still useful.
			/// 
			/// The following list contains all of the heap flags which have any real semantic
			/// meaning for committed resources in the Brawler Engine:
			/// 
			///   - D3D12_HEAP_FLAG_ALLOW_DISPLAY
			///   - D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT
			///   - D3D12_HEAP_FLAG_CREATE_NOT_ZEROED (NOTE: This one is always enabled.)
			/// 
			/// Derived classes can override this function as necessary to specify the flags which
			/// will be passed to the D3D12 API during the creation of a committed resource.
			/// </summary>
			/// <returns>
			/// By default, this function returns D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE. Derived 
			/// classes can override this function as necessary to specify the flags which will be 
			/// passed to the D3D12 API during the creation of a committed resource.
			/// </returns>
			virtual D3D12_HEAP_FLAGS GetCommittedResourceHeapFlags() const;

		public:
			Brawler::D3D12Resource& GetD3D12Resource() const;
			bool IsD3D12ResourceCreated() const;

			/// <summary>
			/// Some resources should not be aliased before they are used on the GPU timeline. For
			/// instance, a buffer in an upload heap is typically used to transfer data from the CPU
			/// to the GPU. These copies are done on the CPU timeline. If the buffer were to be
			/// aliased with another resource before the GPU could access the buffer's data, then the
			/// data would be lost.
			/// 
			/// Derived classes can override this function to describe if/when a particular resource
			/// type can be aliased before it is used on the GPU timeline, noting that if this is the
			/// case, then the contents of the resource before it is used on the GPU timeline are
			/// completely and utterly undefined.
			/// 
			/// Returning false in both this function and I_GPUResource::CanAliasAfterUseOnGPU() will
			/// effectively disable aliasing for a particular resource type.
			/// 
			/// *NOTE*: This function is only used for transient resources. Persistent resources can
			/// never be aliased, regardless of the return value of this function.
			/// </summary>
			/// <returns>
			/// By default, the function returns true, allowing the resource to be aliased before it
			/// is used on the GPU timeline. Typically, we do not care about the contents of a memory
			/// location used by a transient resource before it is used by the GPU.
			/// 
			/// Derived classes can override this function to describe if/when a particular resource
			/// type can be aliased before it is used on the GPU timeline, noting that if this is the
			/// case, then the contents of the resource before it is used on the GPU timeline are
			/// completely and utterly undefined.
			/// </returns>
			virtual bool CanAliasBeforeUseOnGPU() const;

			/// <summary>
			/// There are times when we wish to preserve the contents of a resource after it has been
			/// used by the GPU. For example, a buffer in a readback heap is typically used to transfer
			/// data from the GPU to the CPU. We need to make sure that the data remains intact until
			/// the CPU can copy it over on the CPU timeline.
			/// 
			/// Derived classes can override this function to describe if/when a particular resource
			/// type can be aliased after it is used on the GPU timeline, noting that if this is the
			/// case, then the contents of the resource after it is used on the GPU timeline are
			/// completely and utterly undefined.
			/// 
			/// Returning false in both this function and I_GPUResource::CanAliasBeforeUseOnGPU() will
			/// effectively disable aliasing for a particular resource type.
			/// 
			/// *NOTE*: This function is only used for transient resources. Persistent resources can
			/// never be aliased, regardless of the return value of this function.
			/// </summary>
			/// <returns>
			/// By default, the function returns true, allowing the resource to be aliased after it
			/// is used on the GPU timeline. Typically, we do not care about the contents of a memory
			/// location used by a transient resource after it is used by the GPU.
			/// 
			/// Derived classes can override this function to describe if/when a particular resource
			/// type can be aliased after it is used on the GPU timeline, noting that if this is the
			/// case, then the contents of the resource after it is used on the GPU timeline are
			/// completely and utterly undefined.
			/// </returns>
			virtual bool CanAliasAfterUseOnGPU() const;

			D3D12_RESOURCE_STATES GetSubResourceState(const std::uint32_t subResourceIndex) const;
			std::span<const D3D12_RESOURCE_STATES> GetAllSubResourceStates() const;

			void SetSubResourceState(const D3D12_RESOURCE_STATES newState, const std::uint32_t subResourceIndex);

			const Brawler::D3D12_RESOURCE_DESC& GetResourceDescription() const;
			void SetResourceDescription(Brawler::D3D12_RESOURCE_DESC&& resourceDesc);

			D3D12_HEAP_TYPE GetHeapType() const;
			GPUResourceLifetimeType GetGPUResourceLifetimeType() const;

			const GPUResourceUsageTracker& GetUsageTracker() const;

		private:
			void SetGPUResourceLifetimeType(const GPUResourceLifetimeType lifetimeType);
			void MarkAsUsedForCurrentFrame();

			/// <summary>
			/// When an I_GPUResource instance is assigned a new ID3D12Resource*, its virtual
			/// address changes. This invalidates all descriptors which were previously
			/// referring to the resource.
			/// 
			/// This function is called internally when an I_GPUResource is assigned an
			/// ID3D12Resource* to invalidate all of the old descriptors.
			/// </summary>
			void UpdateDescriptors(Brawler::D3D12Resource& newD3DResource);

		public:
			/// <summary>
			/// Special initialization for an I_GPUResource refers to the fact that the D3D12
			/// API requires textures which are to be used as either render targets or
			/// depth/stencil textures be initialized in one of three ways as their first
			/// operation executed on the GPU timeline:
			/// 
			///   - A discard operation, which is done by calling 
			///     ID3D12GraphicsCommandList::DiscardResource().
			/// 
			///   - A clear operation, which is done by calling either
			///     ID3D12GraphicsCommandList::ClearRenderTargetView() or
			///     ID3D12GraphicsCommandList::ClearDepthStencilView().
			/// 
			///   - A copy operation, which is done by calling one of the
			///     ID3D12GraphicsCommandList::Copy*() functions.
			/// 
			/// In addition, special initialization of I_GPUResource instances is also required
			/// immediately after an aliasing barrier for the resource specified as
			/// ResourceAfter.
			/// 
			/// The Brawler Engine will take care of this special initialization automatically
			/// in all cases which it is required. Derived classes can override this function
			/// to specify the special initialization method which will be used.
			/// 
			/// NOTE: Special initialization is not performed with buffers or textures which
			/// are not to be used as either render targets or depth/stencil textures. For these
			/// types of resources, this function's return value is ignored.
			/// </summary>
			/// <returns>
			/// By default, the function will return GPUResourceSpecialInitializationMethod::DISCARD,
			/// indicating that the I_GPUResource instance should be discarded whenever special
			/// initialization is required.
			/// 
			/// Derived classes can override this function to specify the special initialization
			/// method which will be used.
			/// </returns>
			virtual GPUResourceSpecialInitializationMethod GetPreferredSpecialInitializationMethod() const;

		private:
			bool RequiresSpecialInitialization() const;
			void MarkSpecialInitializationAsCompleted();

		private:
			std::unique_ptr<D3D12ResourceContainer> mResourceContainer;
			GPUResourceLifetimeType mLifetimeType;
			GPUResourceInitializationInfo mInitInfo;
			GPUResourceUsageTracker mUsageTracker;
			GPUSubResourceStateManager mStateManager;
			bool mRequiresSpecialInitialization;
			GPUResourceBindlessSRVManager mBindlessSRVManager;
			mutable std::shared_mutex mResourceCritSection;
		};
	}
}