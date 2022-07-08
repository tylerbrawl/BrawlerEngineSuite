module;
#include "DxDef.h"

export module Util.Engine;
import Brawler.D3D12.BindlessSRVAllocation;
import Brawler.D3D12.GPUVendor;
import Brawler.D3D12.GPUCommandQueueType;
import Brawler.D3D12.GPUCapabilities;

export namespace Brawler
{
	namespace D3D12
	{
		class GPUResourceDescriptorHeap;
		class GPUCommandManager;
		class PresentationManager;
		class PersistentGPUResourceManager;
		class GPUResidencyManager;
		class GPUDevice;
	}
}

export namespace Util
{
	namespace Engine
	{
		constexpr std::size_t MAX_FRAMES_IN_FLIGHT = 2;

		Brawler::D3D12::GPUDevice& GetGPUDevice();

		Brawler::D3D12::GPUCommandManager& GetGPUCommandManager();
		Brawler::D3D12::PresentationManager& GetPresentationManager();
		Brawler::D3D12::PersistentGPUResourceManager& GetPersistentGPUResourceManager();
		Brawler::D3D12::GPUResidencyManager& GetGPUResidencyManager();
		Brawler::D3D12::GPUVendor GetGPUVendor();

		/// <summary>
		/// Gets the "current" frame number for the calling thread. The returned value changes depending on
		/// whether or not said thread is executing a CPU job.
		/// 
		///   - If the current thread *IS* executing a CPU job, then the returned value is the frame number
		///     on which said job was created. This may or may not be the true frame number, but it is
		///     usually what the developer intended on using. (See the documentation of GetTrueFrameNumber() 
		///     for more details.)
		/// 
		///   - If the current thread is *NOT* executing a CPU job, then the returned value is the true
		///     frame number.
		/// 
		/// This implies that the function may, in fact, return a different value when called concurrently
		/// from multiple threads.
		/// </summary>
		/// <returns>
		/// The function returns the "current" frame number for the calling thread. See the summary for
		/// important remarks.
		/// </returns>
		std::uint64_t GetCurrentFrameNumber();
		
		/// <summary>
		/// Gets the "true" frame number. This value represents the current frame number stored within the
		/// renderer, and it is guaranteed to be the same across all threads. It differs from
		/// Util::Engine::GetCurrentFrameNumber() in that it always returns the current frame number, even
		/// if the calling thread is executing a CPU job.
		/// 
		/// In practice, most of the time, you will want to use Util::Engine::GetCurrentFrameNumber(). For
		/// instance, when recording commands into a GPUCommandContext for a RenderPass, we want to write
		/// descriptors into the per-frame descriptor heap segment for the frame number for which the
		/// commands are being submitted, rather than the "true" frame number. Since the true frame number
		/// might be incremented by a separate thread while these commands are being recorded, incorrectly
		/// using Util::Engine::GetTrueFrameNumber() might cause the descriptors to be written into the
		/// wrong per-frame descriptor heap segment as the result of a race condition.
		/// 
		/// On the other hand, Util::Engine::GetTrueFrameNumber() might be more useful for constructing 
		/// low-level engine components. As a general rule of thumb, if the goal is to make sure that some
		/// object can still be used and deciding this involves checking frame numbers, then getting the
		/// "true" frame number is probably the best choice. Of course, this can vary on a case-by-case
		/// basis, and the developer should make their best judgment as to which needs to be used.
		/// </summary>
		/// <returns>
		/// The function returns the "true" frame number, which is guaranteed to be the same across all
		/// threads. See the summary for important remarks.
		/// </returns>
		std::uint64_t GetTrueFrameNumber();

		Brawler::D3D12::GPUResourceDescriptorHeap& GetGPUResourceDescriptorHeap();
		std::uint32_t GetDescriptorHandleIncrementSize(const D3D12_DESCRIPTOR_HEAP_TYPE descriptorType);
		
		Brawler::D3D12Device& GetD3D12Device();
		Brawler::DXGIAdapter& GetDXGIAdapter();
		Brawler::DXGIFactory& GetDXGIFactory();

		/// <summary>
		/// Gets the ID3D12CommandAllocator instance for the calling thread. Since command allocators
		/// are *NOT* free-threaded, each thread in the system is assigned its own set of command
		/// allocators. Specifically, each thread is given Util::Engine::MAX_FRAMES_IN_FLIGHT
		/// ID3D12CommandAllocator instances for each of the three primary queues (DIRECT, COMPUTE,
		/// and COPY).
		/// 
		/// *NOTE*: This function *IS* thread safe.
		/// </summary>
		/// <returns>
		/// The function returns the ID3D12CommandAllocator instance for the calling thread.
		/// </returns>
		Brawler::D3D12CommandAllocator& GetD3D12CommandAllocator(const Brawler::D3D12::GPUCommandQueueType queueType);

		const Brawler::D3D12::GPUCapabilities& GetGPUCapabilities();
	}
}