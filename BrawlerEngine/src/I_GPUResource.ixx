// TODO: Make sure that the heap into which an I_GPUResource is located is made resident
// before it is ever accessed by the GPU!

module;
#include <optional>
#include <mutex>
#include "DxDef.h"

export module Brawler.I_GPUResource;
import Brawler.D3DHeapAllocationHandle;
import Brawler.ResourceCPUVisibleDescriptorManager;
import Brawler.ResourceDescriptorHeapAllocation;
import Brawler.ResourceDescriptorType;

export namespace Brawler
{
	class D3DHeap;
	class D3DHeapManager;
	class I_RenderContext;
	class ResourceTransitionRequestManager;
	class ResourceDescriptorHeap;
}

export namespace Brawler
{
	class I_GPUResource
	{
	private:
		friend class D3DHeap;
		friend class D3DHeapManager;
		friend class I_RenderContext;
		friend class ResourceTransitionRequestManager;
		friend class ResourceDescriptorHeap;

	protected:
		I_GPUResource();

	public:
		virtual ~I_GPUResource();

		I_GPUResource(const I_GPUResource& rhs) = delete;
		I_GPUResource& operator=(const I_GPUResource& rhs) = delete;

		I_GPUResource(I_GPUResource&& rhs) noexcept = default;
		I_GPUResource& operator=(I_GPUResource&& rhs) noexcept = default;

		virtual Brawler::D3D12_RESOURCE_DESC GetResourceDescription() const = 0;

		/// <summary>
		/// Textures which are used as render targets or depth/stencil buffers must
		/// be cleared numerous times throughout the graphics pipeline. By specifying
		/// an optimized clear value, these discard operations can perform faster.
		/// 
		/// By default, the function returns an empty std::optional instance.
		/// 
		/// NOTE: Buffers can never have an optimized clear color. The Brawler Engine
		/// will ignore the returned value if the resource is a buffer.
		/// </summary>
		/// <returns>
		/// The default behavior is to return an empty std::optional instance. However,
		/// this can be overridden by derived classes as needed.
		/// </returns>
		virtual std::optional<D3D12_CLEAR_VALUE> GetOptimizedClearValue() const;

		/// <summary>
		/// It is not possible to infer the correct D3D12_SHADER_RESOURCE_VIEW_DESC from
		/// a resource's D3D12_RESOURCE_DESC alone. If a derived class will ever have
		/// one of its resources be used as an SRV, then it should override this function
		/// to return the correct D3D12_SHADER_RESOURCE_VIEW_DESC.
		/// </summary>
		/// <returns>
		/// The default behavior is to return an empty std::optional instance. However,
		/// this can be overridden by derived classes as needed.
		/// </returns>
		virtual std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> GetSRVDescription() const;

		/// <summary>
		/// In most cases, the appropriate D3D12_UNORDERED_ACCESS_VIEW_DESC can be
		/// constructed by using information provided by I_GPUResource::GetSRVDescription().
		/// This means that overriding I_GPUResource::GetUAVDescription() is typically
		/// unnecessary if a derived class has already overridden
		/// I_GPUResource::GetSRVDescription().
		/// 
		/// However, if a class has not overridden I_GPUResource::GetSRVDescription(), or
		/// if it is desired to manually create the D3D12_UNORDERED_ACCESS_VIEW_DESC for
		/// instances of this class, then this function can be overridden.
		/// </summary>
		/// <returns>
		/// The default behavior depends on the return value of I_GPUResource::GetSRVDescription().
		/// If I_GPUResource::GetSRVDescription() returned an empty std::optional instance,
		/// then this function will also return an empty std::optional instance. Otherwise, it
		/// will return a D3D12_UNORDERED_ACCESS_VIEW_DESC constructed from the
		/// D3D12_SHADER_RESOURCE_VIEW_DESC returned by I_GPUResource::GetSRVDescription().
		/// 
		/// This function can be overridden by derived classes to manually create a 
		/// D3D12_UNORDERED_ACCESS_VIEW_DESC.
		/// </returns>
		virtual std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> GetUAVDescription() const;

		Brawler::D3D12Resource& GetD3D12Resource();
		const Brawler::D3D12Resource& GetD3D12Resource() const;

		D3D12_RESOURCE_STATES GetCurrentResourceState() const;

		/// <summary>
		/// Creates a bindless SRV for this I_GPUResource. This ensures that a per-frame SRV
		/// is not automatically created when binding the resource in an I_RenderContext.
		/// 
		/// NOTE: This *MUST* be called before the resource is ever bound in an I_RenderContext
		/// if it is supposed to be used as a bindless resource!
		/// </summary>
		void CreateBindlessSRV();

	private:
		void InitializeIMPL(Microsoft::WRL::ComPtr<Brawler::D3D12Resource>&& d3dResource, D3DHeapAllocationHandle&& hAllocation, const D3D12_RESOURCE_STATES currentState);
		void SetCurrentResourceState(const D3D12_RESOURCE_STATES currentState);
		CD3DX12_CPU_DESCRIPTOR_HANDLE GetCPUVisibleDescriptorHandle(const ResourceDescriptorType descriptorType) const;
		void SetBindlessSRVAllocation(ResourceDescriptorHeapAllocation&& bindlessAllocation);

	private:
		Microsoft::WRL::ComPtr<Brawler::D3D12Resource> mD3DResource;
		bool mProperlyInitialized;
		D3DHeapAllocationHandle mHAllocation;
		ResourceCPUVisibleDescriptorManager mDescriptorManager;
		std::optional<ResourceDescriptorHeapAllocation> mBindlessSRVAllocation;
		mutable std::mutex mBindlessSRVAllocationCritSection;
		D3D12_RESOURCE_STATES mCurrState;
	};
}