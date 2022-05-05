module;
#include <array>
#include "DxDef.h"

export module Brawler.D3D12.GPUDevice;
import Brawler.D3D12.GPUResourceDescriptorHeap;
import Brawler.D3D12.GPUCapabilities;
import Brawler.D3D12.GPUResidencyManager;
import Brawler.D3D12.GPUVendor;

export namespace Brawler
{
	namespace D3D12
	{
		class GPUDevice
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

		private:
			void InitializeD3D12Device();
			void InitializeGPUVendor();
			void InitializeGPUCapabilities();
			void InitializeDescriptorHandleIncrementSizeArray();

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