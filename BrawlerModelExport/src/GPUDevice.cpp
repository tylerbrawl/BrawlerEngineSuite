module;
#include <cassert>
#include <stdexcept>
#include <array>
#include "DxDef.h"

module Brawler.D3D12.GPUDevice;

namespace
{
	bool VerifyD3D12DeviceFeatureSupport(const Microsoft::WRL::ComPtr<ID3D12Device> d3dDevice)
	{
		// Ensure Shader Model 6.0 support.
		{
			D3D12_FEATURE_DATA_SHADER_MODEL shaderModelData{};
			CheckHRESULT(d3dDevice->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_SHADER_MODEL, &shaderModelData, sizeof(shaderModelData)));

			if (shaderModelData.HighestShaderModel < D3D_SHADER_MODEL::D3D_SHADER_MODEL_6_0)
				return false;
		}

		// Ensure Resource Binding Tier 2 support.
		{
			D3D12_FEATURE_DATA_D3D12_OPTIONS d3d12OptionsData{};
			CheckHRESULT(d3dDevice->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS, &d3d12OptionsData, sizeof(d3d12OptionsData)));

			if (d3d12OptionsData.ResourceBindingTier < D3D12_RESOURCE_BINDING_TIER::D3D12_RESOURCE_BINDING_TIER_2)
				return false;
		}

		return true;
	}
}

namespace Brawler
{
	namespace D3D12
	{
		GPUDevice::GPUDevice() :
			mDXGIFactory(nullptr),
			mDXGIAdapter(nullptr),
			mD3dDevice(nullptr),
			mDescriptorHeap(),
			mHandleIncrementSizeArr()
		{
			InitializeD3D12Device();
			InitializeDescriptorHandleIncrementSizeArray();

			mDescriptorHeap.Initialize();
		}

		Brawler::D3D12Device& GPUDevice::GetD3D12Device()
		{
			return *(mD3dDevice.Get());
		}

		const Brawler::D3D12Device& GPUDevice::GetD3D12Device() const
		{
			return *(mD3dDevice.Get());
		}

		Brawler::DXGIAdapter& GPUDevice::GetDXGIAdapter()
		{
			return *(mDXGIAdapter.Get());
		}

		const Brawler::DXGIAdapter& GPUDevice::GetDXGIAdapter() const
		{
			return *(mDXGIAdapter.Get());
		}

		GPUResourceDescriptorHeap& GPUDevice::GetGPUResourceDescriptorHeap()
		{
			return mDescriptorHeap;
		}

		const GPUResourceDescriptorHeap& GPUDevice::GetGPUResourceDescriptorHeap() const
		{
			return mDescriptorHeap;
		}

		std::uint32_t GPUDevice::GetDescriptorHandleIncrementSize(const D3D12_DESCRIPTOR_HEAP_TYPE descriptorType) const
		{
			return mHandleIncrementSizeArr[descriptorType];
		}

		void GPUDevice::InitializeD3D12Device()
		{
			// Enable the debug layer in Debug builds.
#ifdef _DEBUG
			{
				Microsoft::WRL::ComPtr<ID3D12Debug> debugController{};
				CheckHRESULT(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));

				Microsoft::WRL::ComPtr<ID3D12Debug3> latestDebugController{};
				CheckHRESULT(debugController.As(&latestDebugController));

				latestDebugController->EnableDebugLayer();
			}
#endif // _DEBUG

			// Initialize the DXGI factory.
			{
				Microsoft::WRL::ComPtr<IDXGIFactory2> dxgiFactory{};
				std::uint32_t factoryFlags = 0;

#ifdef _DEBUG
				factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif // _DEBUG

				CheckHRESULT(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&dxgiFactory)));
				CheckHRESULT(dxgiFactory.As(&mDXGIFactory));
			}

			// Get the best possible DXGI adapter and use it to create the D3D12 device.
			{
				Microsoft::WRL::ComPtr<IDXGIAdapter> bestAdapter{};
				std::uint32_t currIndex = 0;

				while (mD3dDevice == nullptr)
				{
					HRESULT hr = mDXGIFactory->EnumAdapterByGpuPreference(
						currIndex++,
						DXGI_GPU_PREFERENCE::DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
						IID_PPV_ARGS(&bestAdapter)
					);

					// Terminate the program if we could not find a valid adapter.
					if (hr == DXGI_ERROR_NOT_FOUND) [[unlikely]]
						throw std::runtime_error{ "ERROR: None of the DXGI adapters present on the system could be created!" };

					Microsoft::WRL::ComPtr<ID3D12Device> d3dDevice{};
					hr = D3D12CreateDevice(
						bestAdapter.Get(),
						D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0,
						IID_PPV_ARGS(&d3dDevice)
					);

					// If we could not create a D3D12Device with this DXGIAdapter, then move on to
					// the next one.
					if (hr != S_OK) [[unlikely]]
						continue;

					// If this device does not support all of our required features, then move on
					// to the next one.
					if (!VerifyD3D12DeviceFeatureSupport(d3dDevice))
						continue;

					CheckHRESULT(bestAdapter.As(&mDXGIAdapter));
					CheckHRESULT(d3dDevice.As(&mD3dDevice));
				}
			}
		}

		void GPUDevice::InitializeDescriptorHandleIncrementSizeArray()
		{
			for (std::size_t i = 0; i < static_cast<std::size_t>(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES); ++i)
				mHandleIncrementSizeArr[i] = mD3dDevice->GetDescriptorHandleIncrementSize(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
		}
	}
}