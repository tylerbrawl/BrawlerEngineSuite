module;
#include <cassert>
#include <stdexcept>
#include <array>
#include "DxDef.h"

module Brawler.D3D12.GPUDevice;
import Brawler.CompositeEnum;
import Util.General;

namespace
{
	static constexpr D3D_FEATURE_LEVEL MINIMUM_D3D_FEATURE_LEVEL = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0;
	
	bool VerifyD3D12DeviceFeatureSupport(const Microsoft::WRL::ComPtr<ID3D12Device> d3dDevice)
	{
		// Ensure Shader Model 6.0 support.
		{
			D3D12_FEATURE_DATA_SHADER_MODEL shaderModelData{
				.HighestShaderModel = D3D_SHADER_MODEL::D3D_SHADER_MODEL_6_0
			};
			Util::General::CheckHRESULT(d3dDevice->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_SHADER_MODEL, &shaderModelData, sizeof(shaderModelData)));

			if (shaderModelData.HighestShaderModel < D3D_SHADER_MODEL::D3D_SHADER_MODEL_6_0)
				return false;
		}

		// Ensure Resource Binding Tier 2 support.
		{
			D3D12_FEATURE_DATA_D3D12_OPTIONS d3d12OptionsData{};
			Util::General::CheckHRESULT(d3dDevice->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS, &d3d12OptionsData, sizeof(d3d12OptionsData)));

			if (d3d12OptionsData.ResourceBindingTier < D3D12_RESOURCE_BINDING_TIER::D3D12_RESOURCE_BINDING_TIER_2)
				return false;
		}

		return true;
	}

	template <Brawler::D3D12::TypedUAVFormat UAVFormat>
	consteval DXGI_FORMAT GetDXGIFormat()
	{
		using namespace Brawler::D3D12;

		// Honestly, as ugly as macros are, it's a lot better than just copying the same
		// case/return statements over and over again.
#define __CHECK_FORMAT__(formatName)	\
case TypedUAVFormat::formatName:		\
	return DXGI_FORMAT::DXGI_FORMAT_ ## formatName
		
		switch (UAVFormat)
		{
			__CHECK_FORMAT__(R32_FLOAT);
			__CHECK_FORMAT__(R32_UINT);
			__CHECK_FORMAT__(R32_SINT);
			__CHECK_FORMAT__(R32G32B32A32_FLOAT);
			__CHECK_FORMAT__(R32G32B32A32_UINT);
			__CHECK_FORMAT__(R32G32B32A32_SINT);
			__CHECK_FORMAT__(R16G16B16A16_FLOAT);
			__CHECK_FORMAT__(R16G16B16A16_UINT);
			__CHECK_FORMAT__(R16G16B16A16_SINT);
			__CHECK_FORMAT__(R8G8B8A8_UNORM);
			__CHECK_FORMAT__(R8G8B8A8_UINT);
			__CHECK_FORMAT__(R8G8B8A8_SINT);
			__CHECK_FORMAT__(R16_FLOAT);
			__CHECK_FORMAT__(R16_UINT);
			__CHECK_FORMAT__(R16_SINT);
			__CHECK_FORMAT__(R8_UNORM);
			__CHECK_FORMAT__(R8_UINT);
			__CHECK_FORMAT__(R8_SINT);
			__CHECK_FORMAT__(R16G16B16A16_UNORM);
			__CHECK_FORMAT__(R16G16B16A16_SNORM);
			__CHECK_FORMAT__(R32G32_FLOAT);
			__CHECK_FORMAT__(R32G32_UINT);
			__CHECK_FORMAT__(R32G32_SINT);
			__CHECK_FORMAT__(R10G10B10A2_UNORM);
			__CHECK_FORMAT__(R10G10B10A2_UINT);
			__CHECK_FORMAT__(R11G11B10_FLOAT);
			__CHECK_FORMAT__(R8G8B8A8_SNORM);
			__CHECK_FORMAT__(R16G16_FLOAT);
			__CHECK_FORMAT__(R16G16_UNORM);
			__CHECK_FORMAT__(R16G16_UINT);
			__CHECK_FORMAT__(R16G16_SNORM);
			__CHECK_FORMAT__(R16G16_SINT);
			__CHECK_FORMAT__(R8G8_UNORM);
			__CHECK_FORMAT__(R8G8_UINT);
			__CHECK_FORMAT__(R8G8_SNORM);
			__CHECK_FORMAT__(R8G8_SINT);
			__CHECK_FORMAT__(R16_UNORM);
			__CHECK_FORMAT__(R16_SNORM);
			__CHECK_FORMAT__(R8_SNORM);
			__CHECK_FORMAT__(A8_UNORM);
			__CHECK_FORMAT__(B5G6R5_UNORM);
			__CHECK_FORMAT__(B5G5R5A1_UNORM);
			__CHECK_FORMAT__(B4G4R4A4_UNORM);

		default:
			return DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		}

#undef __CHECK_FORMAT__
	}

	template <Brawler::D3D12::TypedUAVFormat UAVFormat>
	bool IsTypedUAVLoadSupported(Brawler::D3D12Device& d3dDevice)
	{
		constexpr DXGI_FORMAT CONVERTED_FORMAT = GetDXGIFormat<UAVFormat>();
		D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport{
			.Format = CONVERTED_FORMAT,
			.Support1{},
			.Support2{}
		};

		Util::General::CheckHRESULT(d3dDevice.CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_FORMAT_SUPPORT, &formatSupport, sizeof(formatSupport)));
		return ((formatSupport.Support2 & D3D12_FORMAT_SUPPORT2::D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) != 0);
	}

	void CheckAdditionalTypedUAVLoadFormatSupport(Brawler::D3D12Device& d3dDevice, Brawler::D3D12::GPUCapabilities& deviceCapabilities)
	{
		const auto supportCheckLambda = []<Brawler::D3D12::TypedUAVFormat UAVFormat>(this const auto& self, Brawler::D3D12Device& d3dDevice, Brawler::D3D12::GPUCapabilities& deviceCapabilities)
		{
			if (IsTypedUAVLoadSupported<UAVFormat>(d3dDevice))
				deviceCapabilities.SupportedTypedUAVLoadFormats |= UAVFormat;

			constexpr Brawler::D3D12::TypedUAVFormat NEXT_FORMAT = static_cast<Brawler::D3D12::TypedUAVFormat>(std::to_underlying(UAVFormat) + 1);

			if constexpr (NEXT_FORMAT != Brawler::D3D12::TypedUAVFormat::COUNT_OR_ERROR)
				self.operator()<NEXT_FORMAT>(d3dDevice, deviceCapabilities);
		};

		supportCheckLambda.operator()<Brawler::D3D12::TypedUAVFormat::R16G16B16A16_UNORM>(d3dDevice, deviceCapabilities);
	}
}

namespace Brawler
{
	namespace D3D12
	{
		void GPUDevice::Initialize()
		{
			InitializeD3D12Device();
			InitializeGPUVendor();
			InitializeGPUCapabilities();
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

		GPUResidencyManager& GPUDevice::GetGPUResidencyManager()
		{
			return mResidencyManager;
		}

		const GPUResidencyManager& GPUDevice::GetGPUResidencyManager() const
		{
			return mResidencyManager;
		}

		std::uint32_t GPUDevice::GetDescriptorHandleIncrementSize(const D3D12_DESCRIPTOR_HEAP_TYPE descriptorType) const
		{
			return mHandleIncrementSizeArr[descriptorType];
		}

		GPUVendor GPUDevice::GetGPUVendor() const
		{
			return mVendor;
		}

		const GPUCapabilities& GPUDevice::GetGPUCapabilities() const
		{
			return mDeviceCapabilities;
		}

		void GPUDevice::InitializeD3D12Device()
		{
			// Enable the debug layer in Debug builds.
			if constexpr (Util::General::IsDebugModeEnabled()) 
			{
				Microsoft::WRL::ComPtr<ID3D12Debug> debugController{};
				Util::General::CheckHRESULT(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));

				Microsoft::WRL::ComPtr<ID3D12Debug3> latestDebugController{};
				Util::General::CheckHRESULT(debugController.As(&latestDebugController));

				latestDebugController->EnableDebugLayer();
				latestDebugController->SetEnableGPUBasedValidation(true);
			}

			// Initialize the DXGI factory.
			{
				Microsoft::WRL::ComPtr<IDXGIFactory2> dxgiFactory{};
				std::uint32_t factoryFlags = 0;

				if constexpr (Util::General::IsDebugModeEnabled())
					factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

				Util::General::CheckHRESULT(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&dxgiFactory)));
				Util::General::CheckHRESULT(dxgiFactory.As(&mDXGIFactory));
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
					hr = D3D12CreateDevice(bestAdapter.Get(), MINIMUM_D3D_FEATURE_LEVEL, IID_PPV_ARGS(&d3dDevice));

					// If we could not create a D3D12Device with this DXGIAdapter, then move on to
					// the next one.
					if (hr != S_OK) [[unlikely]]
						continue;

					// If this device does not support all of our required features, then move on
					// to the next one.
					if (!VerifyD3D12DeviceFeatureSupport(d3dDevice)) [[unlikely]]
						continue;

					Util::General::CheckHRESULT(bestAdapter.As(&mDXGIAdapter));
					Util::General::CheckHRESULT(d3dDevice.As(&mD3dDevice));
				}
			}
		}

		void GPUDevice::InitializeGPUVendor()
		{
			DXGI_ADAPTER_DESC adapterDesc{};
			mDXGIAdapter->GetDesc(&adapterDesc);

			// Identify the vendor based on the PCI ID from the adapterDesc. These can be found at 
			// https://pci-ids.ucw.cz/read/PC?restrict=.
			switch (adapterDesc.VendorId)
			{
			case 0x10de:
				mVendor = GPUVendor::NVIDIA;
				break;

				// AMD has two PCI IDs, for some reason. (It's probably for pre-/post- acquisition of 
				// ATI devices, but I am not sure. Hence, we will just list both here.)
			case 0x1002:
			case 0x1022:
				mVendor = GPUVendor::AMD;
				break;

			case 0x8086:
				mVendor = GPUVendor::INTEL;
				break;

			default:
				mVendor = GPUVendor::UNKNOWN;
				break;
			}
		}

		void GPUDevice::InitializeGPUCapabilities()
		{
			static constexpr Brawler::CompositeEnum<TypedUAVFormat> FORMATS_ALWAYS_SUPPORTED = (
				TypedUAVFormat::R32_FLOAT |
				TypedUAVFormat::R32_UINT |
				TypedUAVFormat::R32_SINT
			);
			
			static constexpr Brawler::CompositeEnum<TypedUAVFormat> FORMATS_SUPPORTED_AS_A_SET = (
				TypedUAVFormat::R32G32B32A32_FLOAT |
				TypedUAVFormat::R32G32B32A32_UINT |
				TypedUAVFormat::R32G32B32A32_SINT |
				TypedUAVFormat::R16G16B16A16_FLOAT |
				TypedUAVFormat::R16G16B16A16_UINT |
				TypedUAVFormat::R16G16B16A16_SINT |
				TypedUAVFormat::R8G8B8A8_UNORM |
				TypedUAVFormat::R8G8B8A8_UINT |
				TypedUAVFormat::R8G8B8A8_SINT |
				TypedUAVFormat::R16_FLOAT |
				TypedUAVFormat::R16_UINT |
				TypedUAVFormat::R16_SINT |
				TypedUAVFormat::R8_UNORM |
				TypedUAVFormat::R8_UINT |
				TypedUAVFormat::R8_SINT
			);

			{
				// Set the GPU Resource Heap Tier. This value describes whether or not buffers,
				// render target and depth/stencil textures, and non-render-target and 
				// non-depth/stencil texturs can be placed into the same heap.

				D3D12_FEATURE_DATA_D3D12_OPTIONS optionsData{};
				Util::General::CheckHRESULT(mD3dDevice->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS, &optionsData, sizeof(optionsData)));

				mDeviceCapabilities.GPUResourceHeapTier = (optionsData.ResourceHeapTier == D3D12_RESOURCE_HEAP_TIER::D3D12_RESOURCE_HEAP_TIER_1 ? ResourceHeapTier::TIER_1 : ResourceHeapTier::TIER_2);

				// Check for supported typed UAV formats. The set specified by FORMAT_ALWAYS_SUPPORTED
				// is always guaranteed to exist.

				mDeviceCapabilities.SupportedTypedUAVLoadFormats = FORMATS_ALWAYS_SUPPORTED;

				if (optionsData.TypedUAVLoadAdditionalFormats)
				{
					// If TypedUAVLoadAdditionalFormats == TRUE, then we are guaranteed to support all
					// of the formats in FORMATS_SUPPORTED_AS_A_SET. In addition, we might also be able
					// to support additional formats, so we check for those.

					mDeviceCapabilities.SupportedTypedUAVLoadFormats |= FORMATS_SUPPORTED_AS_A_SET;
					CheckAdditionalTypedUAVLoadFormatSupport(*(mD3dDevice.Get()), mDeviceCapabilities);
				}
			}

			{
				// Set the maximum GPU virtual address (VA) bits per-process. This value is used at
				// runtime to determine whether or not we should bother evicting resources. If this
				// value is too small, then the actual VA range may be the bottleneck of GPU memory usage,
				// and not the actual GPU memory amount.

				D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT vaSupportData{};
				Util::General::CheckHRESULT(mD3dDevice->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT, &vaSupportData, sizeof(vaSupportData)));

				mDeviceCapabilities.MaxGPUVirtualAddressBitsPerProcess = vaSupportData.MaxGPUVirtualAddressBitsPerProcess;
			}

			{
				// Set the dedicated video memory size.
				
				DXGI_ADAPTER_DESC adapterDesc{};
				mDXGIAdapter->GetDesc(&adapterDesc);

				mDeviceCapabilities.DedicatedVideoMemorySizeInBytes = adapterDesc.DedicatedVideoMemory;
			}
		}

		void GPUDevice::InitializeDescriptorHandleIncrementSizeArray()
		{
			for (std::size_t i = 0; i < static_cast<std::size_t>(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES); ++i)
				mHandleIncrementSizeArr[i] = mD3dDevice->GetDescriptorHandleIncrementSize(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
		}
	}
}