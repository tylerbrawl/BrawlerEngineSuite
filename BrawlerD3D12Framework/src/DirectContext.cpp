module;
#include <functional>
#include <limits>
#include <optional>
#include <cassert>
#include "DxDef.h"

module Brawler.D3D12.DirectContext;
import Brawler.D3D12.I_GPUResource;
import Util.Engine;
import Util.General;
import Brawler.D3D12.PresentationManager;
import Brawler.D3D12.GPUResourceRTVDSVHeap;

namespace Brawler
{
	namespace D3D12
	{
		void DirectContext::RecordCommandListIMPL(const std::function<void(DirectContext&)>& recordJob)
		{
			recordJob(*this);
		}

		void DirectContext::Present() const
		{
			// Report that presentation must occur at the end of the current frame to the PresentationManager.
			Util::Engine::GetPresentationManager().EnablePresentationForCurrentFrame();
		}

		void DirectContext::PrepareCommandListIMPL()
		{
			mCurrPSOID.reset();
		}

		void DirectContext::ClearRenderTargetViewIMPL(const I_GPUResource& resource, const D3D12_RENDER_TARGET_VIEW_DESC& rtvDesc) const
		{
			// Always prefer to use the optimized clear value, if it exists. Otherwise, clear the resource
			// view with zeroes.
			std::optional<D3D12_CLEAR_VALUE> optimizedClearValue{ resource.GetOptimizedClearValue() };
			assert((resource.GetResourceDescription().Flags & D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) == 0);

			// Create a temporary RTV.
			GPUResourceRTVHeapAllocation rtvHeapAllocation{ GPUResourceRTVHeap::GetInstance().CreateReservation() };
			const CD3DX12_CPU_DESCRIPTOR_HANDLE hCPUDescriptor{ rtvHeapAllocation.GetAssignedDescriptorHandle() };

			Util::Engine::GetD3D12Device().CreateRenderTargetView(&(resource.GetD3D12Resource()), &rtvDesc, hCPUDescriptor);

			if (optimizedClearValue.has_value()) [[likely]]
			{
				// According to the MSDN, the Format field of the D3D12_CLEAR_VALUE *MUST* match the format
				// for the RTV used to clear the resource. (The source for this information is
				// https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_clear_value.)
				optimizedClearValue->Format = rtvDesc.Format;

				GetCommandList().ClearRenderTargetView(
					hCPUDescriptor,
					optimizedClearValue->Color,
					0,
					nullptr
				);
			}
			else [[unlikely]]
			{
				static constexpr std::array<float, 4> DEFAULT_ZERO_COLOR_ARR{ 0.0f, 0.0f, 0.0f, 0.0f };
				
				GetCommandList().ClearRenderTargetView(
					hCPUDescriptor,
					DEFAULT_ZERO_COLOR_ARR.data(),
					0,
					nullptr
				);
			}
		}

		void DirectContext::PerformSpecialGPUResourceInitialization(I_GPUResource& resource)
		{
			const GPUResourceSpecialInitializationMethod initializationMethod = resource.GetPreferredSpecialInitializationMethod();

			switch (initializationMethod)
			{
			case GPUResourceSpecialInitializationMethod::DISCARD:
			{
				const D3D12_DISCARD_REGION discardRegion{
					.NumRects = 0,
					.pRects = nullptr,
					.FirstSubresource = 0,
					.NumSubresources = static_cast<std::uint32_t>(resource.GetSubResourceCount())
				};

				GetCommandList().DiscardResource(&(resource.GetD3D12Resource()), &discardRegion);

				break;
			}

			case GPUResourceSpecialInitializationMethod::CLEAR:
			{
				const Brawler::D3D12_RESOURCE_DESC& resourceDesc{ resource.GetResourceDescription() };

				const std::optional<D3D12_CLEAR_VALUE> optimizedClearValue{ resource.GetOptimizedClearValue() };
				assert(optimizedClearValue.has_value() && "ERROR: An attempt was made to clear an I_GPUResource for special GPU resource initialization, but its I_GPUResource::GetOptimizedClearValue() function returned an empty std::optional instance!");

				// According to the deep, dark depths of the D3D12 specifications, we are allowed to create a temporary
				// non-shader-visible descriptor heap, use it to create a descriptor, record the descriptor into a
				// command list (via calls to functions like SetRenderTargets()), and then immediately destroy the
				// descriptor heap.
				//
				// Whether or not this is a good idea from a performance perspective has yet to be seen, though.
				//
				// UPDATE: We now have a global RTV descriptor heap and a global DSV descriptor heap. The motivation
				// behind this is that although the D3D12 specifications allow the creation of heaps in the manner
				// mentioned above, in practice, creating too many heaps seems to result in 
				// ID3D12Device::CreateDescriptorHeap() eventually returning E_OUTOFMEMORY, even if the heaps are
				// deleted immediately after creation.
				
				if ((resourceDesc.Flags & D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0)
				{
					GPUResourceRTVHeapAllocation rtvHeapAllocation{ GPUResourceRTVHeap::GetInstance().CreateReservation() };
					Util::Engine::GetD3D12Device().CreateRenderTargetView(&(resource.GetD3D12Resource()), nullptr, rtvHeapAllocation.GetAssignedDescriptorHandle());

					GetCommandList().ClearRenderTargetView(
						rtvHeapAllocation.GetAssignedDescriptorHandle(),
						optimizedClearValue->Color,
						0,
						nullptr
					);
				}

				else if ((resourceDesc.Flags & D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0)
				{
					GPUResourceDSVHeapAllocation dsvHeapAllocation{ GPUResourceDSVHeap::GetInstance().CreateReservation() };
					Util::Engine::GetD3D12Device().CreateDepthStencilView(&(resource.GetD3D12Resource()), nullptr, dsvHeapAllocation.GetAssignedDescriptorHandle());

					GetCommandList().ClearDepthStencilView(
						dsvHeapAllocation.GetAssignedDescriptorHandle(),
						D3D12_CLEAR_FLAGS::D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAGS::D3D12_CLEAR_FLAG_STENCIL,
						optimizedClearValue->DepthStencil.Depth,
						optimizedClearValue->DepthStencil.Stencil,
						0,
						nullptr
					);
				}

				break;
			}

			default:
				break;
			}
		}
	}
}