module;
#include <atomic>

module Brawler.D3D12.Renderer;

namespace Brawler
{
	namespace D3D12
	{
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