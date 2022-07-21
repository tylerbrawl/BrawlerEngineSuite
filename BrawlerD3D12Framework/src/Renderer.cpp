module;
#include <atomic>
#include <cstdint>

module Brawler.D3D12.Renderer;
import Brawler.JobGroup;
import Util.Coroutine;
import Brawler.D3D12.GPUResourceRTVDSVHeap;

namespace Brawler
{
	namespace D3D12
	{
		void Renderer::Initialize()
		{
			// On a separate thread, asynchronously create the bindless SRV index queue. This
			// is a long-running process which involves a lot of heap allocations, but so is
			// initializing the D3D12 device. In fact, the latter takes so long that by doing these
			// two concurrently, creating the bindless SRV index queue should be "free."
			//
			// We also do the same thing for the queues used by the RTV and DSV heaps.
			std::atomic<std::uint32_t> descriptorHeapIndexQueuesInitializedCounter{ 3 };

			{
				Brawler::JobGroup descriptorHeapIndexQueuesInitializationGroup{};
				descriptorHeapIndexQueuesInitializationGroup.Reserve(3);

				descriptorHeapIndexQueuesInitializationGroup.AddJob([this, &descriptorHeapIndexQueuesInitializedCounter] ()
				{
					mDevice.GetGPUResourceDescriptorHeap().InitializeBindlessSRVQueue();

					// We can use std::memory_order::relaxed, because the bindless SRV queue itself
					// is protected by a std::mutex. Similarly, the other queues are also thread safe.
					descriptorHeapIndexQueuesInitializedCounter.fetch_sub(1, std::memory_order::relaxed);
				});

				descriptorHeapIndexQueuesInitializationGroup.AddJob([this, &descriptorHeapIndexQueuesInitializedCounter] ()
				{
					GPUResourceRTVHeap::GetInstance().InitializeAvailableDescriptorIndexQueue();
					descriptorHeapIndexQueuesInitializedCounter.fetch_sub(1, std::memory_order::relaxed);
				});

				descriptorHeapIndexQueuesInitializationGroup.AddJob([this, &descriptorHeapIndexQueuesInitializedCounter] ()
				{
					GPUResourceDSVHeap::GetInstance().InitializeAvailableDescriptorIndexQueue();
					descriptorHeapIndexQueuesInitializedCounter.fetch_sub(1, std::memory_order::relaxed);
				});

				descriptorHeapIndexQueuesInitializationGroup.ExecuteJobsAsync();
			}

			// Initialize the GPUDevice.
			mDevice.Initialize();

			// Now, we will initialize components of the Brawler Engine which require that the
			// GPUDevice be fully initialized, but are otherwise independent of each other. Since
			// these are independent tasks, we can easily parallelize them.
			Brawler::JobGroup postDeviceCreationInitializationGroup{};
			postDeviceCreationInitializationGroup.Reserve(8);

			postDeviceCreationInitializationGroup.AddJob([] ()
			{
				// Load the PSO library from the file system.
				PSODatabase::GetInstance().InitializePSOLibrary();
			});

			postDeviceCreationInitializationGroup.AddJob([this] ()
			{
				// Initialize the PersistentGPUResourceManager.
				mPersistentResourceManager.Initialize();
			});

			postDeviceCreationInitializationGroup.AddJob([this] ()
			{
				// Initialize the GPUCommandManager.
				mCmdManager.Initialize();
			});

			postDeviceCreationInitializationGroup.AddJob([this] ()
			{
				// Initialize the RootSignatureDatabase.
				RootSignatureDatabase::GetInstance();
			});

			postDeviceCreationInitializationGroup.AddJob([this] ()
			{
				// Initialize the FrameGraphManager.
				mFrameGraphManager.Initialize();
			});

			postDeviceCreationInitializationGroup.AddJob([this] ()
			{
				// Initialize the PresentationManager.
				mPresentationManager.Initialize();
			});

			postDeviceCreationInitializationGroup.AddJob([] ()
			{
				// Initialize the ID3D12DescriptorHeap of the GPUResourceRTVHeap.
				GPUResourceRTVHeap::GetInstance().InitializeD3D12DescriptorHeap();
			});

			postDeviceCreationInitializationGroup.AddJob([] ()
			{
				// Initialize the ID3D12DescriptorHeap of the GPUResourceDSVHeap.
				GPUResourceDSVHeap::GetInstance().InitializeD3D12DescriptorHeap();
			});

			postDeviceCreationInitializationGroup.ExecuteJobs();

			// Initialize the PSOs. This *MUST* be done after the RootSignatureDatabase, since PSO
			// compilation relies on compiled root signatures. It also must be done after the PSO
			// library has been loaded from the disk.
			PSODatabase::GetInstance().LoadPSOs();

			// Wait for the bindless SRV, RTV, and DSV index queues to be initialized.
			while (descriptorHeapIndexQueuesInitializedCounter.load(std::memory_order::relaxed) > 0)
				Util::Coroutine::TryExecuteJob();
		}

		GPUCommandManager& Renderer::GetGPUCommandManager()
		{
			return mCmdManager;
		}

		const GPUCommandManager& Renderer::GetGPUCommandManager() const
		{
			return mCmdManager;
		}

		GPUDevice& Renderer::GetGPUDevice()
		{
			return mDevice;
		}

		const GPUDevice& Renderer::GetGPUDevice() const
		{
			return mDevice;
		}

		PersistentGPUResourceManager& Renderer::GetPersistentGPUResourceManager()
		{
			return mPersistentResourceManager;
		}

		const PersistentGPUResourceManager& Renderer::GetPersistentGPUResourceManager() const
		{
			return mPersistentResourceManager;
		}

		PresentationManager& Renderer::GetPresentationManager()
		{
			return mPresentationManager;
		}

		const PresentationManager& Renderer::GetPresentationManager() const
		{
			return mPresentationManager;
		}

		FrameGraphManager& Renderer::GetFrameGraphManager()
		{
			return mFrameGraphManager;
		}

		const FrameGraphManager& Renderer::GetFrameGraphManager() const
		{
			return mFrameGraphManager;
		}

		void Renderer::ProcessFrame()
		{
			// Execute tasks which must be done during each frame, before Renderer::AdvanceFrame()
			// is called.

			mFrameGraphManager.ProcessCurrentFrame();
		}

		void Renderer::AdvanceFrame()
		{
			// Execute tasks which must be done after every frame, noting that it is possible that
			// commands for the frame MAX_FRAMES_IN_FLIGHT behind the new value of mCurrFrameNum
			// might still be recording on other threads. (If an action is sensitive to this condition,
			// then it might be best to instead add it to FrameGraph::ResetFrameGraph(), after
			// FrameGraph::WaitForPreviousFrameGraphExecution() is called.)

			++mCurrFrameNum;
		}

		std::uint64_t Renderer::GetCurrentFrameNumber() const
		{
			return mCurrFrameNum.load();
		}
	}
}