module;
#include <span>
#include <array>
#include <vector>
#include <memory>
#include <DirectXTex.h>

export module Brawler.BC7ImageCompressor;
import Brawler.D3D12.StructuredBufferSubAllocation;
import Brawler.D3D12.ConstantBufferSubAllocation;
import Brawler.D3D12.RenderPassBundle;
import Brawler.D3D12.Texture2D;
import Brawler.D3D12.DescriptorTableBuilder;
import Brawler.D3D12.BufferResource;
import Brawler.D3D12.BufferSubAllocationReservationHandle;

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
			D3D12::StructuredBufferSubAllocation<BufferBC7> OutputBufferSubAllocation;
			D3D12::StructuredBufferSubAllocation<BufferBC7> Error1BufferSubAllocation;
			D3D12::StructuredBufferSubAllocation<BufferBC7> Error2BufferSubAllocation;

			// NOTE: Even though this is a sub-allocation for a constant buffer, we
			// are going to put it into a default heap, since its contents are only
			// ever set once but are read from multiple times.
			D3D12::ConstantBufferSubAllocation<ConstantsBC7> ConstantBufferSubAllocation;

			// ============================================
			// ^ Default Heap | Upload Heap v
			// ============================================

			D3D12::StructuredBufferSubAllocation<ConstantsBC7> ConstantBufferCopySubAllocation;
		};

		struct DescriptorTableBuilderInfo
		{
			D3D12::DescriptorTableBuilder SourceTextureTableBuilder;
			D3D12::DescriptorTableBuilder Error1SRVTableBuilder;
			D3D12::DescriptorTableBuilder Error1UAVTableBuilder;
			D3D12::DescriptorTableBuilder Error2SRVTableBuilder;
			D3D12::DescriptorTableBuilder Error2UAVTableBuilder;
			D3D12::DescriptorTableBuilder OutputTableBuilder;

			DescriptorTableBuilderInfo() :
				SourceTextureTableBuilder(1),
				Error1SRVTableBuilder(1),
				Error1UAVTableBuilder(1),
				Error2SRVTableBuilder(1),
				Error2UAVTableBuilder(1),
				OutputTableBuilder(1)
			{}
		};

	public:
		struct InitInfo
		{
			D3D12::Texture2DSubResource SrcTextureSubResource;
			DXGI_FORMAT DesiredFormat;
		};

	public:
		explicit BC7ImageCompressor(InitInfo&& initInfo);

		BC7ImageCompressor(const BC7ImageCompressor& rhs) = delete;
		BC7ImageCompressor& operator=(const BC7ImageCompressor& rhs) = delete;

		BC7ImageCompressor(BC7ImageCompressor&& rhs) noexcept = default;
		BC7ImageCompressor& operator=(BC7ImageCompressor&& rhs) noexcept = default;

		D3D12::BufferSubAllocationReservationHandle AddCompressionRenderPasses(D3D12::FrameGraphBuilder& frameGraphBuilder);

	private:
		void CreateTransientResources(D3D12::FrameGraphBuilder& frameGraphBuilder);
		void AddImageCompressionRenderPassBundles(D3D12::FrameGraphBuilder& frameGraphBuilder);

		void InitializeBufferResources(D3D12::FrameGraphBuilder& frameGraphBuilder);

		void InitializeDescriptorTableBuilders();

		D3D12::RenderPassBundle CreateResourceUploadRenderPassBundle();
		D3D12::RenderPassBundle CreateCompressionRenderPassBundle();

		std::size_t GetTotalBlockCount() const;

	private:
		InitInfo mInitInfo;
		ResourceInfo mResourceInfo;
		DescriptorTableBuilderInfo mTableBuilderInfo;
	};
}