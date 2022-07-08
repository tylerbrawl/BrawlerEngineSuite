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
		class ReleaseModeDebugLayerEnabler
		{
		public:
			constexpr ReleaseModeDebugLayerEnabler() = default;

			static consteval bool IsDebugLayerAllowed()
			{
				return false;
			}

			static consteval void TryEnableDebugLayer()
			{}

			static consteval bool IsDebugLayerEnabled()
			{
				return false;
			}
		};

		class DebugModeDebugLayerEnabler
		{
		public:
			constexpr DebugModeDebugLayerEnabler() = default;

			static consteval bool IsDebugLayerAllowed();
			void TryEnableDebugLayer();
			bool IsDebugLayerEnabled() const;

		private:
			bool mIsDebugLayerEnabled;
		};

		template <Util::General::BuildMode BuildMode>
		struct DebugLayerEnablerSelector
		{
			using LayerType = ReleaseModeDebugLayerEnabler;
		};

		template <>
		struct DebugLayerEnablerSelector<Util::General::BuildMode::DEBUG>
		{
			using LayerType = DebugModeDebugLayerEnabler;
		};

		using CurrentDebugLayerEnabler = typename DebugLayerEnablerSelector<Util::General::GetBuildMode()>::LayerType;
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

			Brawler::DXGIFactory& GetDXGIFactory();
			const Brawler::DXGIFactory& GetDXGIFactory() const;

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