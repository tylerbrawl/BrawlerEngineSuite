module;
#include <atomic>
#include <span>
#include "DxDef.h"

export module Brawler.ResourceDescriptorHeap;
import Brawler.ThreadSafeQueue;

export namespace Brawler
{
	class ResourceDescriptorHeapAllocation;
	class I_GPUResource;
	class ResourceDescriptorTable;
}

namespace Brawler
{
	namespace IMPL
	{
		// This limit was taken from 
		// https://docs.microsoft.com/en-us/windows/win32/direct3d12/hardware-support#limits-dependant-on-hardware.
		static constexpr std::uint32_t MAX_DESCRIPTORS_IN_SHADER_VISIBLE_HEAP = 1000000;

		// This represents how many bindless SRVs can be in a ResourceDescriptorHeap at once.
		static constexpr std::uint32_t BINDLESS_SRV_SEGMENT_SIZE = 500000;

		// This represents the number of per-frame segments within a ResourceDescriptorHeap. Each per-frame
		// segment contains descriptors which are valid for one frame. This value should equal the maximum
		// number of queued frames allowed by the Brawler Engine.
		static constexpr std::uint32_t PER_FRAME_SEGMENT_COUNT = 2;

		// This represents how many descriptors can be in a per-frame segment of a ResourceDescriptorHeap
		// at once.
		static constexpr std::uint32_t PER_FRAME_SEGMENT_SIZE = (MAX_DESCRIPTORS_IN_SHADER_VISIBLE_HEAP - BINDLESS_SRV_SEGMENT_SIZE) / PER_FRAME_SEGMENT_COUNT;
	}
}

export namespace Brawler
{
	// This is a descriptor heap which supports the following descriptor types:
	//
	//   - CBV
	//   - SRV
	//   - UAV
	//
	// It is always shader-visible/GPU-visible, and is not meant for staging
	// descriptors. For that, one should use Brawler::StagingDescriptorHeap (TODO:
	// not implemented yet).

	class ResourceDescriptorHeap
	{
	public:
		ResourceDescriptorHeap();

		ResourceDescriptorHeap(const ResourceDescriptorHeap& rhs) = delete;
		ResourceDescriptorHeap& operator=(const ResourceDescriptorHeap& rhs) = delete;

		ResourceDescriptorHeap(ResourceDescriptorHeap&& rhs) noexcept = default;
		ResourceDescriptorHeap& operator=(ResourceDescriptorHeap&& rhs) noexcept = default;

		void Initialize();
		void AdvanceFrame();

		/// <summary>
		/// Creates a bindless shader resource view (SRV) for the specified I_GPUResource.
		/// There is a limit (currently 500,000) as to how many bindless SRVs can be placed
		/// in the ResourceDescriptorHeap at once.
		/// </summary>
		/// <param name="resource">
		/// - The resource for which a bindless SRV is to be constructed.
		/// </param>
		void CreateBindlessSRV(I_GPUResource& resource);

		void CreatePerFrameDescriptorTable(ResourceDescriptorTable& descriptorTable);

		void DeleteAllocation(ResourceDescriptorHeapAllocation&& allocation);

	private:
		void CreateDescriptorHeap();
		void InitializeFreeBindlessIndexQueue();

		/// <summary>
		/// Use this function to retrieve the per-frame index tracker for the current
		/// frame. This is used for allocating per-frame descriptors.
		/// </summary>
		/// <returns>
		/// This function returns a reference to the per-frame index tracker for the
		/// current frame.
		/// </returns>
		std::atomic<std::uint32_t>& GetCurrentPerFrameIndexTracker();

		std::uint32_t GetDescriptorHeapIndexFromPerFrameIndex(const std::uint32_t perFrameIndex) const;
		void InitializeDescriptorHandlesForAllocation(ResourceDescriptorHeapAllocation& allocation);

		/// <summary>
		/// Creates a per-frame descriptor heap reservation of numDescriptors descriptors. The returned
		/// value is the index into the total ResourceDescriptorHeap which represents the start of this
		/// range of descriptors.
		/// </summary>
		/// <param name="numDescriptors">
		/// - The number of descriptors to reserve within the current per-frame segment of the
		///   ResourceDescriptorHeap.
		/// </param>
		/// <returns>
		/// The function returns the index into the total ResourceDescriptorHeap which represents the
		/// start of this range of descriptors.
		/// </returns>
		std::uint32_t CreatePerFrameReservation(const std::uint32_t numDescriptors);

	private:
		Microsoft::WRL::ComPtr<Brawler::D3D12DescriptorHeap> mDescriptorHeap;
		ThreadSafeQueue<std::uint32_t, IMPL::BINDLESS_SRV_SEGMENT_SIZE> mFreeBindlessIndexQueue;
		std::array<std::atomic<std::uint32_t>, IMPL::PER_FRAME_SEGMENT_COUNT> mPerFrameIndexArr;
		std::uint32_t mHandleIncrementSize;
	};
}