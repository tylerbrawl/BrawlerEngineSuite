module;
#include <optional>
#include <array>
#include "DxDef.h"

export module Brawler.ResourceCPUVisibleDescriptorManager;
import Brawler.ResourceDescriptorHeapAllocation;
import Brawler.ResourceDescriptorType;

export namespace Brawler
{
	class I_GPUResource;
}

export namespace Brawler
{
	/// <summary>
	/// ResourceCPUVisibleDescriptorManager is a class which coalesces all of the CPU-only descriptors for
	/// an I_GPUResource into one small CPU-visible descriptor heap.
	/// 
	/// Each I_GPUResource instance has its own ResourceCPUVisibleDescriptorManager. When a
	/// ResourceDescriptorHeap needs to create GPU-visible descriptors for an I_GPUResource, it
	/// will actually call ID3D12Device::CopyDescriptorsSimple() to copy the relevant descriptors
	/// from the CPU-only descriptor heap found within the I_GPUResource's
	/// ResourceCPUVisibleDescriptorManager.
	/// </summary>
	class ResourceCPUVisibleDescriptorManager
	{
	public:
		explicit ResourceCPUVisibleDescriptorManager(I_GPUResource& resource);

		void Initialize();

		CD3DX12_CPU_DESCRIPTOR_HANDLE GetCPUVisibleDescriptorHandle(const ResourceDescriptorType descriptorType) const;
		void CreateCPUVisibleDescriptors();

	private:
		void CreateCPUVisibleDescriptorHeap();

	private:
#ifdef _DEBUG
		std::array<bool, 3> mDescriptorInitializedArr;
#endif // _DEBUG

		I_GPUResource* const mOwningResource;
		Microsoft::WRL::ComPtr<Brawler::D3D12DescriptorHeap> mCPUVisibleHeap;
	};
}