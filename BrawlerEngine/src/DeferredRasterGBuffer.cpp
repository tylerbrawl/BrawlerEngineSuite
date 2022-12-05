module;
#include <cassert>
#include <mutex>
#include <DxDef.h>

module Brawler.DeferredRasterGBuffer;
import Brawler.D3D12.Texture2DBuilders;
import Brawler.D3D12.GPUResourceSpecialInitializationMethod;

namespace
{
	consteval Brawler::D3D12::RenderTargetTexture2DBuilder CreateGenericGBufferBuilder(const DXGI_FORMAT textureFormat)
	{
		Brawler::D3D12::RenderTargetTexture2DBuilder gBufferBuilder{};
		gBufferBuilder.SetMipLevelCount(1);
		gBufferBuilder.SetTextureFormat(textureFormat);
		gBufferBuilder.DenyUnorderedAccessViews();
		gBufferBuilder.SetPreferredSpecialInitializationMethod(Brawler::D3D12::GPUResourceSpecialInitializationMethod::CLEAR);

		return gBufferBuilder;
	}

	static constexpr DXGI_FORMAT BASE_COLOR_ROUGHNESS_GBUFFER_FORMAT = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	static constexpr Brawler::D3D12::RenderTargetTexture2DBuilder BASE_COLOR_ROUGHNESS_GBUFFER_BUILDER{ CreateGenericGBufferBuilder(BASE_COLOR_ROUGHNESS_GBUFFER_FORMAT) };

	static constexpr DXGI_FORMAT ENCODED_NORMAL_GBUFFER_FORMAT = DXGI_FORMAT::DXGI_FORMAT_R8G8_UNORM;
	static constexpr Brawler::D3D12::RenderTargetTexture2DBuilder ENCODED_NORMAL_GBUFFER_BUILDER{ CreateGenericGBufferBuilder(ENCODED_NORMAL_GBUFFER_FORMAT) };

	static constexpr DXGI_FORMAT METALLIC_GBUFFER_FORMAT = DXGI_FORMAT::DXGI_FORMAT_R8_UINT;
	static constexpr Brawler::D3D12::RenderTargetTexture2DBuilder METALLIC_GBUFFER_BUILDER{ CreateGenericGBufferBuilder(METALLIC_GBUFFER_FORMAT) };

	static constexpr DXGI_FORMAT DEPTH_BUFFER_FORMAT = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
	static constexpr Brawler::D3D12::DepthStencilTextureBuilder DEPTH_BUFFER_BUILDER{ []()
	{
		Brawler::D3D12::DepthStencilTextureBuilder depthBufferBuilder{};
		depthBufferBuilder.SetMipLevelCount(1);
		depthBufferBuilder.SetTextureFormat(DEPTH_BUFFER_FORMAT);

		// We need to use the depth buffer's data in an SRV in order to perform the shading
		// in a compute shader. The problem is that allowing SRVs in a depth buffer tends to
		// disable compression, which can impact performance.
		//
		// Right now, however, we're only using the depth buffer as an SRV once. Thus, it
		// might be faster to just allow SRVs on it than it would be to copy its contents
		// over to a separate texture in a RenderPass executed before the compute shader.
		depthBufferBuilder.AllowShaderResourceViews();

		// We use reverse Z-depth buffers, so it's worth considering how we want to clear
		// it. Mathematically, we treat reverse Z-depth as mapping the Z-ranges [n, f] in
		// view space to [1, 0] in NDC space, where n and f are the near and far planes,
		// respectively. This is in contrast to traditional depth buffers, which map [n, f]
		// in view space to [0, 1] in NDC space.
		//
		// Regardless of whether we use regular or reverse Z-depth buffers, we want to
		// clear our depth buffers such that any draw within the view frustum in between
		// the clip planes is written to the G-buffers. Thus, we want to clear them to
		// the mapped value for the far plane. That way, anything closer to the camera
		// than the far plane will be rasterized.
		//
		// Using reverse Z-depth, then, we find that our depth buffer should be cleared
		// to Z = 0.0f in NDC space. This is the default behavior provided by the
		// Brawler::D3D12::DepthStencilTextureBuilder instance, so we don't need to
		// call DepthStencilTextureBuilder::SetOptimizedClearValue().
		depthBufferBuilder.SetPreferredSpecialInitializationMethod(Brawler::D3D12::GPUResourceSpecialInitializationMethod::CLEAR);

		return depthBufferBuilder;
	}() };
}

namespace Brawler
{
	void DeferredRasterGBuffer::InitializeTransientResources(BlackboardTransientResourceBuilder& builder)
	{

	}

	D3D12::Texture2D& DeferredRasterGBuffer::GetBaseColorRoughnessGBuffer()
	{
		assert(mBaseColorRoughnessGBufferPtr != nullptr);
		return *mBaseColorRoughnessGBufferPtr;
	}

	const D3D12::Texture2D& DeferredRasterGBuffer::GetBaseColorRoughnessGBuffer() const
	{
		assert(mBaseColorRoughnessGBufferPtr != nullptr);
		return *mBaseColorRoughnessGBufferPtr;
	}

	D3D12::Texture2D& DeferredRasterGBuffer::GetEncodedNormalGBuffer()
	{
		assert(mEncodedNormalGBufferPtr != nullptr);
		return *mEncodedNormalGBufferPtr;
	}

	const D3D12::Texture2D& DeferredRasterGBuffer::GetEncodedNormalGBuffer() const
	{
		assert(mEncodedNormalGBufferPtr != nullptr);
		return *mEncodedNormalGBufferPtr;
	}

	D3D12::Texture2D& DeferredRasterGBuffer::GetMetallicGBuffer()
	{
		assert(mMetallicGBufferPtr != nullptr);
		return *mMetallicGBufferPtr;
	}

	const D3D12::Texture2D& DeferredRasterGBuffer::GetMetallicGBuffer() const
	{
		assert(mMetallicGBufferPtr != nullptr);
		return *mMetallicGBufferPtr
	}

	D3D12::DepthStencilTexture& DeferredRasterGBuffer::GetDepthBuffer()
	{
		assert(mDepthBufferPtr != nullptr);
		return *mDepthBufferPtr;
	}

	const D3D12::DepthStencilTexture& DeferredRasterGBuffer::GetDepthBuffer() const
	{
		assert(mDepthBufferPtr != nullptr);
		return *mDepthBufferPtr;
	}
}