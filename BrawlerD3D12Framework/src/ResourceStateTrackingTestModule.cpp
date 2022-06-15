module;
#include <DirectXMath/DirectXMath.h>
#include "DxDef.h"

module Tests.ResourceStateTrackingTestModule;
import Brawler.D3D12.I_GPUResource;
import Util.Engine;
import Util.Math;
import Brawler.D3D12.BufferResource;
import Brawler.D3D12.BufferResourceInitializationInfo;
import Brawler.D3D12.ConstantBufferSubAllocation;
import Brawler.D3D12.StructuredBufferSubAllocation;
import Brawler.D3D12.UAVCounterSubAllocation;
import Brawler.D3D12.Texture2D;
import Brawler.D3D12.Texture2DBuilders;
import Brawler.D3D12.GPUResourceViews;
import Brawler.D3D12.GPUResourceDescriptorHeap;
import Brawler.D3D12.BindlessSRVAllocation;

import Brawler.D3D12.FrameGraphBuilding;

namespace
{
	static constexpr std::size_t TEST_NUMBER = 3;
	
	class RenderTargetTexture final : public Brawler::D3D12::I_GPUResource
	{
	public:
		// I would never add a parameter that ugly to an actual RenderTargetTexture object constructor!
		// I'm just doing it like this because this is a testing object.
		explicit RenderTargetTexture(const bool makeSimultaneousAccess = false) :
			Brawler::D3D12::I_GPUResource(Brawler::D3D12::GPUResourceInitializationInfo{
				.ResourceDesc{
					.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D,
					.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
					.Width = 1920,
					.Height = 1080,
					.DepthOrArraySize = 1,
					.MipLevels = 1,

					// Think of it like a fat 1080p G-buffer, I guess. It could also be good for storing
					// HDR data in a 1080p texture for the sake of post-processing (assuming the swapchain
					// is also 1080p, of course). Regardless, the actual format doesn't matter too much
					// for our purposes.
					.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT,

					.SampleDesc{
						.Count = 1,
						.Quality = 0
					},
					.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN,

					// NOTE: AMD doesn't recommend using textures as both render targets and in UAVs, nor
					// does it recommend simultaneous-access textures be used as either render targets or
					// depth/stencil textures (see https://gpuopen.com/performance/ under the "Resources"
					// section).
					// 
					// I'm just adding these flags for the sake of potentially creating more possible resource
					// transitions, since that is the point of these unit tests.
					.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS |
						(makeSimultaneousAccess ? D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS : static_cast<D3D12_RESOURCE_FLAGS>(0)),

					.SamplerFeedbackMipRegion{}
				},
				.InitialResourceState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET,
				.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT
			})
		{}

		RenderTargetTexture(const RenderTargetTexture& rhs) = delete;
		RenderTargetTexture& operator=(const RenderTargetTexture& rhs) = delete;

		RenderTargetTexture(RenderTargetTexture&& rhs) noexcept = default;
		RenderTargetTexture& operator=(RenderTargetTexture&& rhs) noexcept = default;
	};

	static constexpr Brawler::D3D12::Texture2DBuilder GENERIC_TEST_TEXTURE2D_BUILDER = [] () -> Brawler::D3D12::Texture2DBuilder
	{
		Brawler::D3D12::Texture2DBuilder builder{};
		builder.SetTextureDimensions(1920, 1080);

		// Think of it like a fat 1080p G-buffer, I guess. It could also be good for storing
		// HDR data in a 1080p texture for the sake of post-processing (assuming the swapchain
		// is also 1080p, of course). Regardless, the actual format doesn't matter too much
		// for our purposes.
		builder.SetTextureFormat(DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_TYPELESS);

		builder.SetInitialResourceState(D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		return builder;
	}();

	void GetCopyableFootprintsTest(const RenderTargetTexture& texture2D)
	{
		const Brawler::D3D12_RESOURCE_DESC& resourceDesc{ texture2D.GetResourceDescription() };

		std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> subresourceFootprintArr{};
		subresourceFootprintArr.resize(texture2D.GetSubResourceCount());

		std::uint32_t numRows = 0;
		std::uint64_t rowSizeInBytesWithoutPadding = 0;
		std::uint64_t totalBytes = 0;

		Brawler::D3D12Device& d3dDevice{ Util::Engine::GetD3D12Device() };

		d3dDevice.GetCopyableFootprints1(
			&resourceDesc,
			0,
			static_cast<std::uint32_t>(texture2D.GetSubResourceCount()),
			0,
			subresourceFootprintArr.data(),
			&numRows,
			&rowSizeInBytesWithoutPadding,
			&totalBytes
		);
	}
}

namespace Tests
{
	void ResourceStateTrackingTestModule::BuildFrameGraph(Brawler::D3D12::FrameGraphBuilder& builder)
	{
		// In an actual I_RenderModule instance, we would likely have ViewComponents from SceneNodes
		// register with it. Then, these ViewComponents would contain views into an I_GPUResource
		// which would be added as a dependency for the FrameGraph. Or something like that.
		//
		// For this, however, we'll just directly create the dummy FrameGraph.
		
		if constexpr (TEST_NUMBER == 1)
			PerformTest1(builder);

		else if constexpr (TEST_NUMBER == 2)
			PerformTest2(builder);

		else if constexpr (TEST_NUMBER == 3)
			PerformTest3(builder);

		else
			assert(false && "ERROR: An invalid TEST_NUMBER was detected for the ResourceStateTrackingTestModule!");
	}

	void ResourceStateTrackingTestModule::PerformTest1(Brawler::D3D12::FrameGraphBuilder& builder) const
	{
		using namespace Brawler::D3D12;

		// First, create the dummy RenderTargetTexture as a transient resource.
		RenderTargetTexture& texture{ builder.CreateTransientResource<RenderTargetTexture>() };

		{
			RenderPassBundle bundle1{};
			
			// Pass 1: texture -> D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			RenderPass<GPUCommandQueueType::DIRECT> pass1{};
			pass1.SetRenderPassName("Render Pass 1");
			pass1.AddResourceDependency(texture, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			
			bundle1.AddDirectRenderPass(std::move(pass1));

			// Pass 2: [No Dependencies]
			RenderPass<GPUCommandQueueType::DIRECT> pass2{};
			pass2.SetRenderPassName("Render Pass 2");

			bundle1.AddDirectRenderPass(std::move(pass2));

			// Pass 3: texture -> D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			RenderPass<GPUCommandQueueType::DIRECT> pass3{};
			pass3.SetRenderPassName("Render Pass 3");
			pass3.AddResourceDependency(texture, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

			bundle1.AddDirectRenderPass(std::move(pass3));

			builder.AddRenderPassBundle(std::move(bundle1));
		}

		{
			RenderPassBundle bundle2{};

			// Pass 4: texture -> D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			RenderPass<GPUCommandQueueType::DIRECT> pass4{};
			pass4.SetRenderPassName("Render Pass 4");
			pass4.AddResourceDependency(texture, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			bundle2.AddDirectRenderPass(std::move(pass4));

			// Pass 5: texture -> D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			RenderPass<GPUCommandQueueType::COMPUTE> pass5{};
			pass5.SetRenderPassName("Render Pass 5");
			pass5.AddResourceDependency(texture, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

			bundle2.AddComputeRenderPass(std::move(pass5));

			// Pass 6: [No Dependencies]
			RenderPass<GPUCommandQueueType::COMPUTE> pass6{};
			pass6.SetRenderPassName("Render Pass 6");

			bundle2.AddComputeRenderPass(std::move(pass6));

			builder.AddRenderPassBundle(std::move(bundle2));
		}

		{
			RenderPassBundle bundle3{};

			// Pass 7: [No Dependencies]
			RenderPass<GPUCommandQueueType::DIRECT> pass7{};
			pass7.SetRenderPassName("Render Pass 7");

			bundle3.AddDirectRenderPass(std::move(pass7));

			// Pass 8: texture -> D3D12_RESOURCE_STATE_UNORDERED_ACCESS
			RenderPass<GPUCommandQueueType::DIRECT> pass8{};
			pass8.SetRenderPassName("Render Pass 8");
			pass8.AddResourceDependency(texture, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			bundle3.AddDirectRenderPass(std::move(pass8));

			// Pass 9: [No Dependencies]
			RenderPass<GPUCommandQueueType::DIRECT> pass9{};
			pass9.SetRenderPassName("Render Pass 9");

			bundle3.AddDirectRenderPass(std::move(pass9));

			// Pass 10: [No Dependencies]
			RenderPass<GPUCommandQueueType::DIRECT> pass10{};
			pass10.SetRenderPassName("Render Pass 10");

			bundle3.AddDirectRenderPass(std::move(pass10));

			// Pass 11: texture -> D3D12_RESOURCE_STATE_RENDER_TARGET
			RenderPass<GPUCommandQueueType::DIRECT> pass11{};
			pass11.SetRenderPassName("Render Pass 11");
			pass11.AddResourceDependency(texture, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET);

			bundle3.AddDirectRenderPass(std::move(pass11));

			// Pass 12: texture -> D3D12_RESOURCE_STATE_PRESENT
			RenderPass<GPUCommandQueueType::DIRECT> pass12{};
			pass12.SetRenderPassName("Render Pass 12");
			pass12.AddResourceDependency(texture, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT);

			bundle3.AddDirectRenderPass(std::move(pass12));

			builder.AddRenderPassBundle(std::move(bundle3));
		}
	}

	void ResourceStateTrackingTestModule::PerformTest2(Brawler::D3D12::FrameGraphBuilder& builder) const
	{
		using namespace Brawler::D3D12;

		// First, create the dummy RenderTargetTexture object.
		RenderTargetTexture& texture{ builder.CreateTransientResource<RenderTargetTexture>(true) };

		{
			RenderPassBundle bundle1{};

			// Pass 1: texture -> D3D12_RESOURCE_STATE_UNORDERED_ACCESS
			RenderPass<GPUCommandQueueType::DIRECT> pass1{};
			pass1.SetRenderPassName("Render Pass 1");
			pass1.AddResourceDependency(texture, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			bundle1.AddDirectRenderPass(std::move(pass1));

			// Pass 2: [No Dependencies]
			RenderPass<GPUCommandQueueType::DIRECT> pass2{};
			pass2.SetRenderPassName("Render Pass 2");
			
			bundle1.AddDirectRenderPass(std::move(pass2));

			builder.AddRenderPassBundle(std::move(bundle1));
		}

		// =======================================================================================
		// STATE DECAY BARRIER
		//		texture -> D3D12_RESOURCE_STATE_COMMON
		// =======================================================================================

		{
			RenderPassBundle bundle2{};

			// Pass 3: texture -> D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			RenderPass<GPUCommandQueueType::DIRECT> pass3{};
			pass3.SetRenderPassName("Render Pass 3");
			pass3.AddResourceDependency(texture, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			bundle2.AddDirectRenderPass(std::move(pass3));

			// Pass 4: texture -> D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			RenderPass<GPUCommandQueueType::COMPUTE> pass4{};
			pass4.SetRenderPassName("Render Pass 4");
			pass4.AddResourceDependency(texture, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

			bundle2.AddComputeRenderPass(std::move(pass4));

			builder.AddRenderPassBundle(std::move(bundle2));
		}

		// =======================================================================================
		// STATE DECAY BARRIER
		//		texture -> D3D12_RESOURCE_STATE_COMMON
		// =======================================================================================

		{
			RenderPassBundle bundle3{};

			// Pass 5: texture -> D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			RenderPass<GPUCommandQueueType::DIRECT> pass5{};
			pass5.SetRenderPassName("Render Pass 5");
			pass5.AddResourceDependency(texture, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			bundle3.AddDirectRenderPass(std::move(pass5));

			// Pass 6: texture -> D3D12_RESOURCE_STATE_COPY_SOURCE
			RenderPass<GPUCommandQueueType::DIRECT> pass6{};
			pass6.SetRenderPassName("Render Pass 6");
			pass6.AddResourceDependency(texture, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE);

			bundle3.AddDirectRenderPass(std::move(pass6));

			// Pass 7: [No Dependencies]
			RenderPass<GPUCommandQueueType::DIRECT> pass7{};
			pass7.SetRenderPassName("Render Pass 7");
			
			bundle3.AddDirectRenderPass(std::move(pass7));

			// Pass 8: [No Dependencies]
			RenderPass<GPUCommandQueueType::DIRECT> pass8{};
			pass8.SetRenderPassName("Render Pass 8");

			bundle3.AddDirectRenderPass(std::move(pass8));

			// Pass 9: texture -> D3D12_RESOURCE_STATE_RENDER_TARGET
			RenderPass<GPUCommandQueueType::DIRECT> pass9{};
			pass9.SetRenderPassName("Render Pass 9");
			pass9.AddResourceDependency(texture, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET);

			bundle3.AddDirectRenderPass(std::move(pass9));

			builder.AddRenderPassBundle(std::move(bundle3));
		}

		// =======================================================================================
		// STATE DECAY BARRIER
		//		texture -> D3D12_RESOURCE_STATE_COMMON
		// =======================================================================================
	}

	void ResourceStateTrackingTestModule::PerformTest3(Brawler::D3D12::FrameGraphBuilder& builder) const
	{
		using namespace Brawler::D3D12;

		struct Constants_T
		{
			DirectX::XMFLOAT4 Position;
			float Radius;

			float __Pad0;
			float __Pad1;
			float __Pad2;
		};

		BufferResource& bufferResource{ builder.CreateTransientResource<BufferResource>(BufferResourceInitializationInfo{
			.SizeInBytes = Util::Math::KilobytesToBytes(128),
			.InitialResourceState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT
		}) };

		std::optional<ConstantBufferSubAllocation<Constants_T>> optionalCBAllocation{ bufferResource.CreateBufferSubAllocation<ConstantBufferSubAllocation<Constants_T>>() };
		assert(optionalCBAllocation.has_value());

		ConstantBufferSubAllocation<Constants_T> cbAllocation1{ std::move(*optionalCBAllocation) };

		optionalCBAllocation = bufferResource.CreateBufferSubAllocation<ConstantBufferSubAllocation<Constants_T>>();
		assert(optionalCBAllocation.has_value());

		ConstantBufferSubAllocation<Constants_T> cbAllocation2{ std::move(*optionalCBAllocation) };

		std::optional<StructuredBufferSubAllocation<Constants_T>> optionalStructuredBufferAllocation{ bufferResource.CreateBufferSubAllocation<StructuredBufferSubAllocation<Constants_T>>(69u) };
		assert(optionalStructuredBufferAllocation.has_value());

		StructuredBufferSubAllocation<Constants_T> structuredBufferAllocation{ std::move(*optionalStructuredBufferAllocation) };

		std::optional<UAVCounterSubAllocation> optionalUAVCounterAllocation{ bufferResource.CreateBufferSubAllocation<UAVCounterSubAllocation>() };
		assert(optionalUAVCounterAllocation.has_value());

		structuredBufferAllocation.SetUAVCounter(std::move(*optionalUAVCounterAllocation));

		Texture2D& texture2DResource{ builder.CreateTransientResource<Texture2D>(GENERIC_TEST_TEXTURE2D_BUILDER) };
		auto texture2DSrv{ texture2DResource.CreateShaderResourceView<DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT>() };

		BindlessSRVAllocation bindlessTexture2DAllocation{ texture2DSrv.MakeBindless() };

		Util::General::DebugBreak();
	}
}