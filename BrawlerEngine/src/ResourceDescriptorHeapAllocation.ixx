module;
#include <cstddef>
#include <optional>
#include "DxDef.h"

export module Brawler.ResourceDescriptorHeapAllocation;
import Brawler.DescriptorHandleInfo;

namespace Brawler
{
	class ResourceDescriptorHeap;
}

export namespace Brawler
{
	/// <summary>
	/// A ResourceDescriptorHeapAllocation denotes an allocation into a ResourceDescriptorHeap. As
	/// of writing this, the only ResourceDescriptorHeap in the Brawler Engine is a member of
	/// Brawler::Renderer. ResourceDescriptorHeapAllocation instances can only be constructed from
	/// within a ResourceDescriptorHeap (i.e., its member functions).
	/// 
	/// If the allocation is valid, then this means that it refers to a descriptor within a (the)
	/// GPU-visible descriptor heap. (All ResourceDescriptorHeap instances refer to GPU-visible
	/// descriptor heaps.) One can check if this is the case by calling
	/// ResourceDescriptorHeapAllocation::IsAllocationValid().
	/// 
	/// NOTE: ResourceDescriptorHeapAllocations do *NOT* refer to CPU-only descriptors! Each
	/// I_GPUResource instance has a ResourceCPUVisibleDescriptorManager which is responsible for creating
	/// CPU descriptors for the resource; these are then used to stage descriptors onto a
	/// ResourceDescriptorHeap so that the GPU can view them.
	/// </summary>
	class ResourceDescriptorHeapAllocation
	{
	private:
		friend class ResourceDescriptorHeap;

	private:
		explicit ResourceDescriptorHeapAllocation(const std::uint32_t heapIndex);

	public:
		~ResourceDescriptorHeapAllocation();

		ResourceDescriptorHeapAllocation(const ResourceDescriptorHeapAllocation& rhs) = delete;
		ResourceDescriptorHeapAllocation& operator=(const ResourceDescriptorHeapAllocation& rhs) = delete;

		ResourceDescriptorHeapAllocation(ResourceDescriptorHeapAllocation&& rhs) noexcept;
		ResourceDescriptorHeapAllocation& operator=(ResourceDescriptorHeapAllocation&& rhs) noexcept;

		/// <summary>
		/// Checks to see if this ResourceDescriptorHeapAllocation is still a valid allocation; that is,
		/// if the descriptor which this instance refers to can be used as expected.
		/// </summary>
		/// <returns>
		/// The function returns true if this ResourceDescriptorHeapAllocation is still a valid allocation
		/// and false otherwise.
		/// </returns>
		bool IsAllocationValid() const;

		/// <summary>
		/// Retrieves the CPU and GPU descriptor handles for this ResourceDescriptorHeapAllocation,
		/// assuming that this allocation is still valid. (The function asserts if it is not.)
		/// </summary>
		/// <returns>
		/// The function returns a const& to the DescriptorHandleInfo which contains the CPU and GPU
		/// descriptor handles for this ResourceDescriptorHeapAllocation.
		/// </returns>
		const DescriptorHandleInfo& GetDescriptorHandles() const;

	private:
		void DeleteAllocation();

	private:
		std::optional<std::uint32_t> mHeapIndex;
		DescriptorHandleInfo mHandleInfo;
	};
}