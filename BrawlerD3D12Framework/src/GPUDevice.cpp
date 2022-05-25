module;
#include <cassert>
#include <stdexcept>
#include <array>
#include <string_view>
#include <optional>
#include <format>
#include "DxDef.h"

module Brawler.D3D12.GPUDevice;
import Brawler.CompositeEnum;
import Util.General;
import Util.D3D12;
import Util.Win32;
import Brawler.Win32.DLLManager;
import Brawler.Win32.SafeModule;

namespace
{
	static constexpr D3D_FEATURE_LEVEL MINIMUM_D3D_FEATURE_LEVEL = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0;
	
	static constexpr bool ALLOW_WARP_DEVICE = false;
	
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

	// #including pix3.h is causing compile times in Release builds to become miserably slow. (I'm talking, like,
	// hours for a single module interface unit here.) I blame C++20 modules and the MSVC's support for them.
	//
	// So, if PIX isn't included, then we'll just define what we need here ourselves to allow the program to
	// compile.

#ifndef PIX_EVENTS_ARE_TURNED_ON
	static __forceinline HMODULE PIXLoadLatestWinPixGpuCapturerLibrary()
	{}
#endif // !ARE_PIX_EVENTS_TURNED_ON

	void VerifyPIXCapturerLoaded()
	{
		if constexpr (Util::D3D12::IsPIXRuntimeSupportEnabled())
		{
			// First, check to see if the capturer DLL was already loaded. This will be the case if the
			// application is launched directly from PIX.
			const std::optional<HMODULE> hPIXModule{ Brawler::Win32::DLLManager::GetInstance().GetExistingModule(L"WinPixGpuCapturer.dll") };

			// If that failed, then we need to load it ourselves. PIX provides a helper function to do
			// just that.
			if (!hPIXModule.has_value())
			{
				Brawler::Win32::SafeModule pixModule{ PIXLoadLatestWinPixGpuCapturerLibrary() };

				if (pixModule == nullptr) [[unlikely]]
					throw std::runtime_error{ std::format("ERROR: PIX failed to load the WinPixGpuCapturer.dll file at runtime with the following error: {}", Util::General::WStringToString(Util::Win32::GetLastErrorString())) };

				Brawler::Win32::DLLManager::GetInstance().RegisterModule(std::move(pixModule));
			}
		}
	}
}

namespace Brawler
{
	namespace D3D12
	{
		consteval bool DebugModeDebugLayerEnabler::IsDebugLayerAllowed()
		{
			// The NVIDIA debug layer driver sucks. It seems to shoot out debug layer errors and dereference nullptrs for no
			// good reason. So, even if we are building for Debug mode, this setting can be toggled to either enable or
			// disable the D3D12 debug layer.
			// 
			// Honestly, I have a really shaky relationship with this debug layer. As of writing this, I just tried out
			// PIX's debug layer, and it seems to be leaps and bounds ahead of this buggy mess. My current recommendation
			// is to just capture a frame with PIX and use its debug layer offline, setting ALLOW_D3D12_DEBUG_LAYER to be
			// false.
			// 
			// (It's honestly incredibly discomforting that the PIX and NVIDIA debug layers produce different results.
			// After the jank which I've dealt with using the NVIDIA debug layer, I'm inclined to trust the PIX debug layer
			// messages more, but who really knows which is better?)
			//
			// NOTE: The debug layer is never enabled in Release builds, even if this value is set to true.
			constexpr bool ALLOW_D3D12_DEBUG_LAYER = false;

			return ALLOW_D3D12_DEBUG_LAYER;
		}

		void DebugModeDebugLayerEnabler::TryEnableDebugLayer()
		{
			// If we get to this point, then the Debug Layer must be allowed for this build. So, we
			// will make a best effort to enable it here.

			// PIX does not play nicely with the D3D12 Debug Layer. If we detect that PIX is trying
			// to capture the application, then we will not enable the Debug Layer. (I am not aware
			// of a way to detect PIX timing captures, but this should work for GPU captures.)
			{
				static constexpr std::wstring_view PIX_CAPTURER_DLL_NAME{ L"WinPixGpuCapturer.dll" };

				HMODULE hPixCapturerModule = nullptr;
				const bool getPixModuleResult = GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, PIX_CAPTURER_DLL_NAME.data(), &hPixCapturerModule);

				// If we found the PIX capturer module, then do not load the D3D12 Debug Layer.
				if (getPixModuleResult) [[unlikely]]
				{
					Util::Win32::WriteDebugMessage(L"WARNING: The D3D12 Debug Layer was set to be enabled (see DebugLayerEnabler<Util::General::BuildMode::DEBUG>::IsDebugLayerAllowed()), but PIX is attempting to perform a GPU capture. Thus, the request to enable the D3D12 Debug Layer has been ignored. If you wish, you may use PIX's Debug Layer after the capture has completed.");
					mIsDebugLayerEnabled = false;
				}
			}

			{
				Microsoft::WRL::ComPtr<ID3D12Debug> oldDebugController{};
				HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&oldDebugController));

				if (FAILED(hr)) [[unlikely]]
					mIsDebugLayerEnabled = false;

				Microsoft::WRL::ComPtr<Brawler::D3D12Debug> latestDebugController{};
				hr = oldDebugController.As(&latestDebugController);

				if (FAILED(hr)) [[unlikely]]
					mIsDebugLayerEnabled = false;

				latestDebugController->EnableDebugLayer();

				// They weren't kidding when they said that GPU-based validation slows down your
				// program significantly. Enable this at your own risk. Honestly, you're probably
				// better off just using GPU-based validation on a PIX capture, going to sleep, and
				// then looking at the results when you wake up.
				static constexpr bool ENABLE_GPU_BASED_VALIDATION = false;

				latestDebugController->SetEnableGPUBasedValidation(ENABLE_GPU_BASED_VALIDATION);
			}

			mIsDebugLayerEnabled = true;
		}

		bool DebugModeDebugLayerEnabler::IsDebugLayerEnabled() const
		{
			return mIsDebugLayerEnabled;
		}
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

			mDescriptorHeap.InitializeD3D12DescriptorHeap();
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

		bool GPUDevice::IsDebugLayerEnabled() const
		{
			if constexpr (!CurrentDebugLayerEnabler::IsDebugLayerAllowed())
				return false;
			else
				return CurrentDebugLayerEnabler::IsDebugLayerEnabled();
		}

		void GPUDevice::InitializeD3D12Device()
		{
			// Enable PIX captures in Debug and Release with Debugging builds.
			if constexpr (Util::D3D12::IsPIXRuntimeSupportEnabled())
				VerifyPIXCapturerLoaded();
			
			// Enable the debug layer in Debug builds.
			if constexpr (CurrentDebugLayerEnabler::IsDebugLayerAllowed())
				CurrentDebugLayerEnabler::TryEnableDebugLayer();

			// Initialize the DXGI factory.
			{
				Microsoft::WRL::ComPtr<IDXGIFactory2> dxgiFactory{};
				std::uint32_t factoryFlags = 0;

				if constexpr (Util::General::IsDebugModeEnabled())
					factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

				Util::General::CheckHRESULT(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&dxgiFactory)));
				Util::General::CheckHRESULT(dxgiFactory.As(&mDXGIFactory));
			}

			// Create the D3D12 device.
			{
				if constexpr (Util::General::IsDebugModeEnabled() && ALLOW_WARP_DEVICE)
					CreateWARPD3D12Device();
				else
					CreateHardwareD3D12Device();
			}

			// In Debug builds, edit the Debug Layer messages and breakpoints.
			if constexpr (CurrentDebugLayerEnabler::IsDebugLayerAllowed())
			{
				if (CurrentDebugLayerEnabler::IsDebugLayerEnabled())
				{
					// Add D3D12 Debug Layer warnings which you wish to ignore here V. You better have a good reason for
					// doing so, though, and you should document it well.

					static std::array<D3D12_MESSAGE_ID, 2> ignoredD3D12DebugLayerMessageArr{
						// Disable error messages for invalid alignments from ID3D12Device::GetResourceAllocationInfo2().
						// We already check if an alignment value is invalid when calling that function, and revert
						// to standard (non-small) alignment when this happens. As a result, this error message becomes
						// redundant. (In fact, I'm not entirely sure why this is even an error message in the debug
						// layer in the first place, considering that the function returns an error value if the small
						// alignment cannot be used, anyways.)
						D3D12_MESSAGE_ID::D3D12_MESSAGE_ID_CREATERESOURCE_INVALIDALIGNMENT,

						// Disable warnings for multiple buffer resources having the same GPU virtual address. This is
						// a natural consequence of the means by which we alias transient resources.
						//
						// I'm not entirely sure why this warning exists. The idea is probably to ensure that the
						// developer is aware that buffers which share GPU memory cannot be used simultaneously in
						// different states, but the FrameGraph system of the Brawler Engine guarantees that this does
						// not happen.
						D3D12_MESSAGE_ID::D3D12_MESSAGE_ID_HEAP_ADDRESS_RANGE_INTERSECTS_MULTIPLE_BUFFERS
					};

					D3D12_INFO_QUEUE_FILTER infoQueueFilter{};
					infoQueueFilter.DenyList.NumIDs = static_cast<std::uint32_t>(ignoredD3D12DebugLayerMessageArr.size());

					// Some genius made pIDList a non-const D3D12_MESSAGE_ID*, so ignoredD3D12DebugLayerMessageArr can't
					// be constexpr.
					infoQueueFilter.DenyList.pIDList = ignoredD3D12DebugLayerMessageArr.data();

					Microsoft::WRL::ComPtr<ID3D12InfoQueue> d3dInfoQueue{};
					Util::General::CheckHRESULT(mD3dDevice.As(&d3dInfoQueue));

					Util::General::CheckHRESULT(d3dInfoQueue->AddStorageFilterEntries(&infoQueueFilter));

					// Do a debug break on any D3D12 error messages with a severity >= D3D12_MESSAGE_SEVERITY_WARNING.
					Util::General::CheckHRESULT(d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY::D3D12_MESSAGE_SEVERITY_WARNING, true));
					Util::General::CheckHRESULT(d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY::D3D12_MESSAGE_SEVERITY_ERROR, true));
					Util::General::CheckHRESULT(d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY::D3D12_MESSAGE_SEVERITY_CORRUPTION, true));
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

		void GPUDevice::CreateHardwareD3D12Device()
		{
			// Get the best possible DXGI adapter and use it to create the D3D12 device.
			
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

		void GPUDevice::CreateWARPD3D12Device()
		{
			Microsoft::WRL::ComPtr<IDXGIAdapter> warpAdapter{};
			Util::General::CheckHRESULT(mDXGIFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

			Microsoft::WRL::ComPtr<ID3D12Device> warpD3dDevice{};
			Util::General::CheckHRESULT(D3D12CreateDevice(warpAdapter.Get(), MINIMUM_D3D_FEATURE_LEVEL, IID_PPV_ARGS(&warpD3dDevice)));

			if (!VerifyD3D12DeviceFeatureSupport(warpD3dDevice)) [[unlikely]]
				throw std::runtime_error{ "ERROR: The WARP device could not support all of the required D3D12 features!" };

			Util::General::CheckHRESULT(warpAdapter.As(&mDXGIAdapter));
			Util::General::CheckHRESULT(warpD3dDevice.As(&mD3dDevice));
		}
	}
}