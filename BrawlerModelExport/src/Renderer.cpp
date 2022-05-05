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
			// Initialize the main D3D12MA::Allocator instance.
			{
				const D3D12MA::ALLOCATOR_DESC allocatorDesc{
				.pDevice = &(mDevice.GetD3D12Device()),
				.pAdapter = &(mDevice.GetDXGIAdapter())
				};

				CheckHRESULT(D3D12MA::CreateAllocator(&allocatorDesc, &mD3D12MAAllocator));
			}

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

		D3D12MA::Allocator& Renderer::GetD3D12Allocator()
		{
			return *(mD3D12MAAllocator.Get());
		}

		const D3D12MA::Allocator& Renderer::GetD3D12Allocator() const
		{
			return *(mD3D12MAAllocator.Get());
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