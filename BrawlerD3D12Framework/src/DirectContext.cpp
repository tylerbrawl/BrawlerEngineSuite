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

namespace
{
	template <D3D12_DESCRIPTOR_HEAP_TYPE DescriptorType>
	Microsoft::WRL::ComPtr<Brawler::D3D12DescriptorHeap> CreateDefaultDescriptorHeap()
	{
		static constexpr D3D12_DESCRIPTOR_HEAP_DESC HEAP_DESC{
			.Type = DescriptorType,
			.NumDescriptors = 1,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			.NodeMask = 0
		};

		Microsoft::WRL::ComPtr<Brawler::D3D12DescriptorHeap> descriptorHeap{};
		Util::General::CheckHRESULT(Util::Engine::GetD3D12Device().CreateDescriptorHeap(&HEAP_DESC, IID_PPV_ARGS(&descriptorHeap)));

		return descriptorHeap;
	}
}

namespace Brawler
{
	namespace D3D12
	{
		void DirectContext::RecordCommandListIMPL(const std::function<void(DirectContext&)>& recordJob)
		{
			recordJob(*this);
		}

		void DirectContext::PrepareCommandListIMPL()
		{
			mCurrPSOID.reset();
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
				
				if ((resourceDesc.Flags & D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0)
				{
					const Microsoft::WRL::ComPtr<Brawler::D3D12DescriptorHeap> defaultRTVHeap{ CreateDefaultDescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV>() };
					Util::Engine::GetD3D12Device().CreateRenderTargetView(&(resource.GetD3D12Resource()), nullptr, defaultRTVHeap->GetCPUDescriptorHandleForHeapStart());

					GetCommandList().ClearRenderTargetView(
						defaultRTVHeap->GetCPUDescriptorHandleForHeapStart(),
						optimizedClearValue->Color,
						0,
						nullptr
					);
				}

				else if ((resourceDesc.Flags & D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0)
				{
					const Microsoft::WRL::ComPtr<Brawler::D3D12DescriptorHeap> defaultDSVHeap{ CreateDefaultDescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV>() };
					Util::Engine::GetD3D12Device().CreateDepthStencilView(&(resource.GetD3D12Resource()), nullptr, defaultDSVHeap->GetCPUDescriptorHandleForHeapStart());

					GetCommandList().ClearDepthStencilView(
						defaultDSVHeap->GetCPUDescriptorHandleForHeapStart(),
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