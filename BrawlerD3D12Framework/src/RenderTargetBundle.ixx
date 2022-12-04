module;
#include <array>
#include <cassert>
#include <ranges>
#include "DxDef.h"

export module Brawler.D3D12.RenderTargetBundle;
import Brawler.D3D12.GPUResourceRTVDSVHeap;
import Brawler.D3D12.GPUResourceViews;
import Brawler.D3D12.I_GPUResource;
import Util.Engine;

export namespace Brawler
{
	namespace D3D12
	{
		template <std::size_t RTVCount, std::size_t DSVCount>
		class RenderTargetBundle
		{
		private:
			static_assert(RTVCount <= D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT, "ERROR: An attempt was made to instantiate a RenderTargetBundle class template with more render target views than the D3D12 API supports at once!");
			static_assert(DSVCount <= 1, "ERROR: An attempt was made to instantiate a RenderTargetBundle class template with more than one depth/stencil view!");

		private:
			struct RTVInfo
			{
				const I_GPUResource* GPUResourcePtr;
				D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
				bool NeedsDescriptorHeapUpdate;
			};

			struct DSVInfo
			{
				const I_GPUResource* GPUResourcePtr;
				D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc;
				bool NeedsDescriptorHeapUpdate;
			};

		public:
			RenderTargetBundle();

			RenderTargetBundle(const RenderTargetBundle& rhs) = delete;
			RenderTargetBundle& operator=(const RenderTargetBundle& rhs) = delete;

			RenderTargetBundle(RenderTargetBundle&& rhs) noexcept = default;
			RenderTargetBundle& operator=(RenderTargetBundle&& rhs) noexcept = default;

			template <std::size_t RTVIndex, DXGI_FORMAT Format, D3D12_RTV_DIMENSION ViewDimension>
			void CreateRenderTargetView(const RenderTargetView<Format, ViewDimension>& rtv);

			template <std::size_t DSVIndex, DXGI_FORMAT Format, D3D12_DSV_DIMENSION ViewDimension, DepthStencilAccessMode AccessMode>
			void CreateDepthStencilView(const DepthStencilView<Format, ViewDimension, AccessMode>& dsv);

			std::array<CD3DX12_CPU_DESCRIPTOR_HANDLE, RTVCount> GetCPUDescriptorHandleArrayForRTVs();
			std::array<CD3DX12_CPU_DESCRIPTOR_HANDLE, DSVCount> GetCPUDescriptorHandleArrayForDSVs();

		private:
			void CreateDescriptorHeapAllocations();

		private:
			std::array<RTVInfo, RTVCount> mRTVInfoArr;
			std::array<GPUResourceRTVHeapAllocation, RTVCount> mRTVHeapAllocationArr;
			std::array<DSVInfo, DSVCount> mDSVInfoArr;
			std::array<GPUResourceDSVHeapAllocation, DSVCount> mDSVHeapAllocationArr;
		};
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <std::size_t RTVCount, std::size_t DSVCount>
		RenderTargetBundle<RTVCount, DSVCount>::RenderTargetBundle() :
			mRTVInfoArr(),
			mRTVHeapAllocationArr(),
			mDSVInfoArr(),
			mDSVHeapAllocationArr()
		{
			CreateDescriptorHeapAllocations();
		}

		template <std::size_t RTVCount, std::size_t DSVCount>
		template <std::size_t RTVIndex, DXGI_FORMAT Format, D3D12_RTV_DIMENSION ViewDimension>
		void RenderTargetBundle<RTVCount, DSVCount>::CreateRenderTargetView(const RenderTargetView<Format, ViewDimension>& rtv)
		{
			static_assert(RTVIndex < mRTVInfoArr.size(), "ERROR: An attempt was made to specify an out-of-bounds index for a render target view in a call to RenderTargetBundle::CreateRenderTargetView()!");

			mRTVInfoArr[RTVIndex] = RTVInfo{
				.GPUResourcePtr = &(rtv.GetGPUResource()),
				.RTVDesc{ rtv.CreateRTVDescription() },
				.NeedsDescriptorHeapUpdate = true
			};
		}

		template <std::size_t RTVCount, std::size_t DSVCount>
		template <std::size_t DSVIndex, DXGI_FORMAT Format, D3D12_DSV_DIMENSION ViewDimension, DepthStencilAccessMode AccessMode>
		void RenderTargetBundle<RTVCount, DSVCount>::CreateDepthStencilView(const DepthStencilView<Format, ViewDimension, AccessMode>& dsv)
		{
			static_assert(DSVIndex < mDSVInfoArr.size(), "ERROR: An attempt was made to specify an out-of-bounds index for a depth/stencil view in a call to RenderTargetBundle::CreateDepthStencilView()!");

			mDSVInfoArr[DSVIndex] = DSVInfo{
				.GPUResourcePtr = &(dsv.GetGPUResource()),
				.DSVDesc{ dsv.CreateDSVDescription() },
				.NeedsDescriptorHeapUpdate = true
			};
		}

		template <std::size_t RTVCount, std::size_t DSVCount>
		std::array<CD3DX12_CPU_DESCRIPTOR_HANDLE, RTVCount> RenderTargetBundle<RTVCount, DSVCount>::GetCPUDescriptorHandleArrayForRTVs()
		{
			// For each RTVInfo, ensure that the GPUResourceRTVHeap has the current descriptor
			// at the location specified by the corresponding GPUResourceRTVHeapAllocation instance.
			//
			// Unfortunately, as of writing this, we STILL don't have std::views::zip.

			std::array<CD3DX12_CPU_DESCRIPTOR_HANDLE, RTVCount> hRTVArr{};

			for (const auto i : std::views::iota(0ull, RTVCount))
			{
				RTVInfo& currRTVInfo{ mRTVInfoArr[i] };
				const GPUResourceRTVHeapAllocation& currRTVHeapAllocation{ mRTVHeapAllocationArr[i] };

				assert(currRTVInfo.GPUResourcePtr != nullptr && "ERROR: A RenderTargetBundle instance did not have all of its RTVs initialized prior to a call to RenderTargetBundle::GetCPUDescriptorHandleArrayForRTVs()!");

				const CD3DX12_CPU_DESCRIPTOR_HANDLE hCurrRTV{ currRTVHeapAllocation.GetAssignedDescriptorHandle() };

				if (currRTVInfo.NeedsDescriptorHeapUpdate) [[unlikely]]
				{
					Util::Engine::GetD3D12Device().CreateRenderTargetView(&(currRTVInfo.GPUResourcePtr->GetD3D12Resource()), &(currRTVInfo.RTVDesc), hCurrRTV);
					currRTVInfo.NeedsDescriptorHeapUpdate = false;
				}

				hRTVArr[i] = hCurrRTV;
			}

			return hRTVArr;
		}

		template <std::size_t RTVCount, std::size_t DSVCount>
		std::array<CD3DX12_CPU_DESCRIPTOR_HANDLE, DSVCount> RenderTargetBundle<RTVCount, DSVCount>::GetCPUDescriptorHandleArrayForDSVs()
		{
			// For each DSVInfo, ensure that the GPUResourceDSVHeap has the current descriptor
			// at the location specified by the corresponding GPUResourceDSVHeapAllocation instance.
			//
			// Unfortunately, as of writing this, we STILL don't have std::views::zip.

			std::array<CD3DX12_CPU_DESCRIPTOR_HANDLE, DSVCount> hDSVArr{};

			for (const auto i : std::views::iota(0ull, DSVCount))
			{
				DSVInfo& currDSVInfo{ mDSVInfoArr[i] };
				const GPUResourceDSVHeapAllocation& currDSVHeapAllocation{ mDSVHeapAllocationArr[i] };

				assert(currDSVInfo.GPUResourcePtr != nullptr && "ERROR: A RenderTargetBundle instance did not have all of its DSVs initialized prior to a call to RenderTargetBundle::GetCPUDescriptorHandleArrayForDSVs()!");

				const CD3DX12_CPU_DESCRIPTOR_HANDLE hCurrDSV{ currDSVHeapAllocation.GetAssignedDescriptorHandle() };

				if (currDSVInfo.NeedsDescriptorHeapUpdate) [[unlikely]]
				{
					Util::Engine::GetD3D12Device().CreateDepthStencilView(&(currDSVInfo.GPUResourcePtr->GetD3D12Resource()), &(currDSVInfo.DSVDesc), hCurrDSV);
					currDSVInfo.NeedsDescriptorHeapUpdate = false;
				}

				hDSVArr[i] = hCurrDSV;
			}

			return hDSVArr;
		}

		template <std::size_t RTVCount, std::size_t DSVCount>
		void RenderTargetBundle<RTVCount, DSVCount>::CreateDescriptorHeapAllocations()
		{
			// Upon creating a RenderTargetBundle instance, allocate the necessary views from the
			// GPUResourceRTVHeap and the GPUResourceDSVHeap.

			for (auto& rtvHeapAllocation : mRTVHeapAllocationArr)
				rtvHeapAllocation = GPUResourceRTVHeap::GetInstance().CreateReservation();

			for (auto& dsvHeapAllocation : mDSVHeapAllocationArr)
				dsvHeapAllocation = GPUResourceDSVHeap::GetInstance().CreateReservation();
		}
	}
}