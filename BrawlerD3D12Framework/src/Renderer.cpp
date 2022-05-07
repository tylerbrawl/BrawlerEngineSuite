module;
#include <atomic>

module Brawler.D3D12.Renderer;

namespace Brawler
{
	namespace D3D12
	{
		void Renderer::Initialize()
		{
			// Initialize the GPUDevice.
			mDevice.Initialize();

			// Initialize the PersistentGPUResourceManager.
			mPersistentResourceManager.Initialize();

			// Initialize the GPUCommandManager.
			mCmdManager.Initialize();

			// Initialize the RootSignatureDatabase.
			RootSignatureDatabase::GetInstance();

			// Initialize the PSODatabase. This *MUST* be initialized after the RootSignatureDatabase,
			// since PSO compilation relies on compiled root signatures.
			PSODatabase::GetInstance();
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

		void Renderer::AdvanceFrame()
		{
			// Execute tasks which must be done after every frame.
			{
				mDevice.GetGPUResourceDescriptorHeap().ResetPerFrameDescriptorHeapIndex();
			}

			++mCurrFrameNum;
		}

		std::uint64_t Renderer::GetCurrentFrameNumber() const
		{
			return mCurrFrameNum.load();
		}
	}
}