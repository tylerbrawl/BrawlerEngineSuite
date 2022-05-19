module;
#include "DxDef.h"

export module Util.Engine;
import Brawler.D3D12.BindlessSRVAllocation;

export namespace Brawler
{
	namespace D3D12
	{
		class GPUResourceDescriptorHeap;
		struct GPUCapabilities;
		class GPUCommandManager;
		class PersistentGPUResourceManager;
		class GPUResidencyManager;
	}
}

export namespace Util
{
	namespace Engine
	{
		constexpr std::size_t MAX_FRAMES_IN_FLIGHT = 2;

		Brawler::D3D12::GPUCommandManager& GetGPUCommandManager();

		Brawler::D3D12::PersistentGPUResourceManager& GetPersistentGPUResourceManager();

		Brawler::D3D12::GPUResidencyManager& GetGPUResidencyManager();

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
		/// low-level engine components.
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

		const Brawler::D3D12::GPUCapabilities& GetGPUCapabilities();
	}
}