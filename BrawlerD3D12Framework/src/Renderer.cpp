module;
#include <atomic>
#include "DxDef.h"

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
			mRSDatabase.InitializeDatabase();

			// Initialize the PSODatabase. This *MUST* be done after initializing the
			// RootSignatureDatabase.
			mPSODatabase.Initialize();
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

		RootSignatureDatabase& Renderer::GetRootSignatureDatabase()
		{
			return mRSDatabase;
		}

		const RootSignatureDatabase& Renderer::GetRootSignatureDatabase() const
		{
			return mRSDatabase;
		}

		PSODatabase& Renderer::GetPSODatabase()
		{
			return mPSODatabase;
		}

		const PSODatabase& Renderer::GetPSODatabase() const
		{
			return mPSODatabase;
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