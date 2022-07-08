module;
#include <atomic>

module Brawler.D3D12.Renderer;
import Brawler.JobGroup;
import Util.Coroutine;

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
			std::atomic<bool> bindlessSRVQueueInitialized{ false };

			{
				Brawler::JobGroup bindlessSRVQueueInitializationGroup{};

				bindlessSRVQueueInitializationGroup.AddJob([this, &bindlessSRVQueueInitialized] ()
				{
					mDevice.GetGPUResourceDescriptorHeap().InitializeBindlessSRVQueue();

					// We can use std::memory_order::relaxed, because the bindless SRV queue itself
					// is protected by a std::mutex.
					bindlessSRVQueueInitialized.store(true, std::memory_order::relaxed);
				});

				bindlessSRVQueueInitializationGroup.ExecuteJobsAsync();
			}

			// Initialize the GPUDevice.
			mDevice.Initialize();

			// Now, we will initialize components of the Brawler Engine which require that the
			// GPUDevice be fully initialized, but are otherwise independent of each other. Since
			// these are independent and thread-safe tasks, we can easily parallelize them.
			Brawler::JobGroup postDeviceCreationInitializationGroup{};
			postDeviceCreationInitializationGroup.Reserve(6);

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

			postDeviceCreationInitializationGroup.ExecuteJobs();

			// Initialize the PSOs. This *MUST* be done after the RootSignatureDatabase, since PSO
			// compilation relies on compiled root signatures. It also must be done after the PSO
			// library has been loaded from the disk.
			PSODatabase::GetInstance().LoadPSOs();

			// Wait for the bindless SRV queue to be initialized.
			while (!bindlessSRVQueueInitialized.load(std::memory_order::relaxed))
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