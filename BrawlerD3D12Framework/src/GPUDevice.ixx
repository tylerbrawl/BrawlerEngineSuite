module;
#include <array>
#include "DxDef.h"

export module Brawler.D3D12.GPUDevice;
import Brawler.D3D12.GPUResourceDescriptorHeap;
import Brawler.D3D12.GPUCapabilities;
import Brawler.D3D12.GPUResidencyManager;
import Brawler.D3D12.GPUVendor;
import Util.General;

namespace Brawler
{
	namespace D3D12
	{
		template <Util::General::BuildMode BuildMode>
		class DebugLayerEnabler
		{
		public:
			constexpr DebugLayerEnabler() = default;

			static consteval bool IsDebugLayerAllowed()
			{
				return false;
			}
		};

		bool TryEnableD3D12DebugLayer();

		template <>
		class DebugLayerEnabler<Util::General::BuildMode::DEBUG>
		{
		public:
			DebugLayerEnabler() = default;

			static consteval bool IsDebugLayerAllowed()
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

			void TryEnableDebugLayer()
			{
				mIsDebugLayerEnabled = TryEnableD3D12DebugLayer();
			}

			bool IsDebugLayerEnabled() const
			{
				return mIsDebugLayerEnabled;
			}

		private:
			bool mIsDebugLayerEnabled;
		};

		using CurrentDebugLayerEnabler = DebugLayerEnabler<Util::General::GetBuildMode()>;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class GPUDevice final : private CurrentDebugLayerEnabler
		{
		public:
			GPUDevice() = default;

			GPUDevice(const GPUDevice& rhs) = delete;
			GPUDevice& operator=(const GPUDevice& rhs) = delete;

			GPUDevice(GPUDevice&& rhs) noexcept = default;
			GPUDevice& operator=(GPUDevice&& rhs) noexcept = default;

			void Initialize();

			Brawler::D3D12Device& GetD3D12Device();
			const Brawler::D3D12Device& GetD3D12Device() const;

			Brawler::DXGIAdapter& GetDXGIAdapter();
			const Brawler::DXGIAdapter& GetDXGIAdapter() const;

			GPUResourceDescriptorHeap& GetGPUResourceDescriptorHeap();
			const GPUResourceDescriptorHeap& GetGPUResourceDescriptorHeap() const;

			GPUResidencyManager& GetGPUResidencyManager();
			const GPUResidencyManager& GetGPUResidencyManager() const;

			std::uint32_t GetDescriptorHandleIncrementSize(const D3D12_DESCRIPTOR_HEAP_TYPE descriptorType) const;

			GPUVendor GetGPUVendor() const;

			const GPUCapabilities& GetGPUCapabilities() const;

			bool IsDebugLayerEnabled() const;

		private:
			void InitializeD3D12Device();
			void InitializeGPUVendor();
			void InitializeGPUCapabilities();
			void InitializeDescriptorHandleIncrementSizeArray();

			void CreateHardwareD3D12Device();
			void CreateWARPD3D12Device();

		private:
			Microsoft::WRL::ComPtr<Brawler::DXGIFactory> mDXGIFactory;
			Microsoft::WRL::ComPtr<Brawler::DXGIAdapter> mDXGIAdapter;
			Microsoft::WRL::ComPtr<Brawler::D3D12Device> mD3dDevice;
			GPUResourceDescriptorHeap mDescriptorHeap;
			GPUResidencyManager mResidencyManager;
			std::array<std::uint32_t, std::to_underlying(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES)> mHandleIncrementSizeArr;
			GPUVendor mVendor;
			GPUCapabilities mDeviceCapabilities;
		};
	}
}