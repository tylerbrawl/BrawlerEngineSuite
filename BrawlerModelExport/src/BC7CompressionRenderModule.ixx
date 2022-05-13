module;
#include <memory>
#include <vector>
#include <assimp/material.h>
#include <DirectXTex.h>

export module Brawler.BC7CompressionRenderModule;
import Brawler.D3D12.I_RenderModule;
import Brawler.ThreadSafeVector;
import Brawler.BC7ImageCompressor;
import Brawler.BC7CompressionEventHandle;
import Brawler.TextureTypeMap;

namespace Brawler
{
	template <DXGI_FORMAT Format>
	consteval bool IsBC7CompressedFormat()
	{
		switch (Format)
		{
		case DXGI_FORMAT::DXGI_FORMAT_BC7_TYPELESS: [[fallthrough]];
		case DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM: [[fallthrough]];
		case DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM_SRGB:
			return true;

		default:
			return false;
		}
	}

	template <aiTextureType TextureType>
	concept IsBC7Compressible = IsBC7CompressedFormat<Brawler::GetDesiredTextureFormat<TextureType>()>();
}

export namespace Brawler
{
	class BC7CompressionRenderModule final : public D3D12::I_RenderModule
	{
	public:
		BC7CompressionRenderModule() = default;

		BC7CompressionRenderModule(const BC7CompressionRenderModule& rhs) = delete;
		BC7CompressionRenderModule& operator=(const BC7CompressionRenderModule& rhs) = delete;

		BC7CompressionRenderModule(BC7CompressionRenderModule&& rhs) noexcept = default;
		BC7CompressionRenderModule& operator=(BC7CompressionRenderModule&& rhs) noexcept = default;

		template <aiTextureType TextureType>
			requires IsBC7Compressible<TextureType>
		BC7CompressionEventHandle MakeGPUCompressionRequest(const DirectX::Image& srcImage);

	protected:
		void BuildFrameGraph(D3D12::FrameGraphBuilder& builder) override;

	private:
		void HandlePendingCompressionRequests(D3D12::FrameGraphBuilder& builder);
		void CheckActiveCompressorsForDeletion();

	private:
		ThreadSafeVector<std::unique_ptr<BC7ImageCompressor>> mPendingCompressorPtrArr;
		std::vector<std::unique_ptr<BC7ImageCompressor>> mActiveCompressorPtrArr;
	};
}

// ----------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <aiTextureType TextureType>
		requires IsBC7Compressible<TextureType>
	BC7CompressionEventHandle BC7CompressionRenderModule::MakeGPUCompressionRequest(const DirectX::Image& srcImage)
	{
		std::unique_ptr<BC7ImageCompressor> imageCompressorPtr{ std::make_unique<BC7ImageCompressor>(BC7ImageCompressor::InitInfo{
			.SrcImage{srcImage},
			.DesiredFormat{Brawler::GetDesiredTextureFormat<TextureType>()}
		}) };
		const BC7CompressionEventHandle hCompressionEvent{ *imageCompressorPtr };

		mPendingCompressorPtrArr.PushBack(std::move(imageCompressorPtr));

		return hCompressionEvent;
	}
}