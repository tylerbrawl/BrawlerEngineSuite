module;
#include <span>
#include <array>
#include <vector>
#include <memory>
#include <DirectXTex.h>

export module Brawler.BC7ImageCompressor;
import Brawler.D3D12.StructuredBufferSubAllocation;
import Brawler.D3D12.ConstantBufferSubAllocation;
import Brawler.D3D12.TextureCopyBufferSubAllocation;
import Brawler.D3D12.RenderPassBundle;
import Brawler.D3D12.Texture2D;

namespace Brawler
{
	struct ConstantsBC7
	{
		std::uint32_t TextureWidth;
		std::uint32_t NumBlockX;
		std::uint32_t Format;
		std::uint32_t NumTotalBlocks;
		float AlphaWeight;
		float __Pad0;
		float __Pad1;
		float __Pad2;
	};
}

export namespace Brawler
{
	namespace D3D12
	{
		class FrameGraphBuilder;
	}
}

export namespace Brawler
{
	class BC7ImageCompressor
	{
	private:
		struct BufferBC7
		{
			DirectX::XMUINT4 Color;
		};

		struct ResourceInfo
		{
			// ============================================
			// v Default Heap
			// ============================================
			D3D12::Texture2D* SourceTexturePtr;

			D3D12::StructuredBufferSubAllocation<BufferBC7> OutputBufferSubAllocation;
			D3D12::StructuredBufferSubAllocation<BufferBC7> Error1BufferSubAllocation;
			D3D12::StructuredBufferSubAllocation<BufferBC7> Error2BufferSubAllocation;

			// ============================================
			// ^ Default Heap | Upload Heap v
			// ============================================

			D3D12::ConstantBufferSubAllocation<ConstantsBC7> ConstantBufferSubAllocation;
			D3D12::TextureCopyBufferSubAllocation SourceTextureCopySubAllocation;

			// ============================================
			// ^ Upload Heap | Readback Heap v
			// ============================================

			D3D12::StructuredBufferSubAllocation<BufferBC7> CPUOutputBufferSubAllocation;
		};

	public:
		struct InitInfo
		{
			DirectX::Image& DestImage;
			const DirectX::Image& SrcImage;
			DXGI_FORMAT DesiredFormat;
		};

	public:
		explicit BC7ImageCompressor(InitInfo&& initInfo);

		BC7ImageCompressor(const BC7ImageCompressor& rhs) = delete;
		BC7ImageCompressor& operator=(const BC7ImageCompressor& rhs) = delete;

		BC7ImageCompressor(BC7ImageCompressor&& rhs) noexcept = default;
		BC7ImageCompressor& operator=(BC7ImageCompressor&& rhs) noexcept = default;

		void CreateTransientResources(D3D12::FrameGraphBuilder& frameGraphBuilder);
		std::vector<D3D12::RenderPassBundle> GetImageCompressionRenderPassBundles() const;

		//void CompressImage(DirectX::Image& destImage);

	private:
		void InitializeBufferResources(D3D12::FrameGraphBuilder& frameGraphBuilder);
		void InitializeSourceTextureResource(D3D12::FrameGraphBuilder& frameGraphBuilder);

		D3D12::RenderPassBundle CreateSourceTextureInitializationRenderPassBundle() const;
		D3D12::RenderPassBundle CreateCompressionRenderPassBundle() const;

	private:
		InitInfo mInitInfo;
		ResourceInfo mResourceInfo;


		//std::unique_ptr<D3D12::StructuredBuffer<BufferBC7>> mOutputBuffer;
		//std::unique_ptr<D3D12::StructuredBuffer<BufferBC7>> mError1Buffer;
		//std::unique_ptr<D3D12::StructuredBuffer<BufferBC7>> mError2Buffer;

		//std::unique_ptr<D3D12::ConstantBuffer<ConstantsBC7>> mConstantBuffer;
		
		//std::unique_ptr<D3D12::StructuredBuffer<BufferBC7>> mOutputCPUBuffer;

		//D3D12::PerFrameDescriptorTable mError1SRV;
		//D3D12::PerFrameDescriptorTable mError2SRV;
		//D3D12::PerFrameDescriptorTable mOutputUAV;
		//D3D12::PerFrameDescriptorTable mError1UAV;
		//D3D12::PerFrameDescriptorTable mError2UAV;
	};
}