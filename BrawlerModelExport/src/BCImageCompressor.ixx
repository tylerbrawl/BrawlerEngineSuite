module;
#include <span>
#include <array>
#include <vector>
#include <memory>
#include <DirectXTex.h>

export module Brawler.BCImageCompressor;
import Brawler.D3D12.StructuredBuffer;
import Brawler.D3D12.ConstantBuffer;
import Brawler.D3D12.GPUResourceDescriptors.PerFrameDescriptorTable;

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
	class BCImageCompressor
	{
	private:
		struct BufferBC7
		{
			std::array<std::uint32_t, 4> Color;
		};

	public:
		BCImageCompressor(const DirectX::Image& srcImage, const DXGI_FORMAT desiredFormat);

		BCImageCompressor(const BCImageCompressor& rhs) = delete;
		BCImageCompressor& operator=(const BCImageCompressor& rhs) = delete;

		BCImageCompressor(BCImageCompressor&& rhs) noexcept = default;
		BCImageCompressor& operator=(BCImageCompressor&& rhs) noexcept = default;

		void CompressImage(DirectX::Image& destImage);

	private:
		void InitializeBuffers();

	private:
		const DirectX::Image* mSrcImagePtr;
		DXGI_FORMAT mDesiredFormat;

		std::unique_ptr<D3D12::StructuredBuffer<BufferBC7>> mOutputBuffer;
		std::unique_ptr<D3D12::StructuredBuffer<BufferBC7>> mError1Buffer;
		std::unique_ptr<D3D12::StructuredBuffer<BufferBC7>> mError2Buffer;

		std::unique_ptr<D3D12::ConstantBuffer<ConstantsBC7>> mConstantBuffer;
		
		std::unique_ptr<D3D12::StructuredBuffer<BufferBC7>> mOutputCPUBuffer;

		D3D12::PerFrameDescriptorTable mError1SRV;
		D3D12::PerFrameDescriptorTable mError2SRV;
		D3D12::PerFrameDescriptorTable mOutputUAV;
		D3D12::PerFrameDescriptorTable mError1UAV;
		D3D12::PerFrameDescriptorTable mError2UAV;
	};
}