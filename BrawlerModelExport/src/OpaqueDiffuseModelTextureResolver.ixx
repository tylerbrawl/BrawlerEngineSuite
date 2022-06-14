module;
#include <optional>
#include <memory>
#include <array>
#include <vector>
#include <DirectXTex.h>

export module Brawler.OpaqueDiffuseModelTextureResolver;
import Brawler.ModelTextureResolutionEventHandle;
import Brawler.ImportedMesh;
import Brawler.D3D12.BufferResource;
import Brawler.D3D12.StructuredBufferSubAllocation;
import Brawler.D3D12.FrameGraphBuilder;
import Brawler.D3D12.Texture2D;
import Brawler.D3D12.BufferSubAllocationReservationHandle;
import Brawler.BC7ImageCompressor;

export namespace Brawler
{
	class OpaqueDiffuseModelTextureResolver
	{
	private:
		struct TextureResolutionContext
		{
			D3D12::FrameGraphBuilder& Builder;
			D3D12::Texture2D* CurrTexturePtr;
			std::vector<D3D12::BufferSubAllocationReservationHandle> HBC7TextureDataReservationArr;
		};

		struct BufferBC7
		{
			DirectX::XMUINT4 Color;
		};

	public:
		explicit OpaqueDiffuseModelTextureResolver(const ImportedMesh& mesh);

		OpaqueDiffuseModelTextureResolver(const OpaqueDiffuseModelTextureResolver& rhs) = delete;
		OpaqueDiffuseModelTextureResolver& operator=(const OpaqueDiffuseModelTextureResolver& rhs) = delete;

		OpaqueDiffuseModelTextureResolver(OpaqueDiffuseModelTextureResolver&& rhs) noexcept = default;
		OpaqueDiffuseModelTextureResolver& operator=(OpaqueDiffuseModelTextureResolver&& rhs) noexcept = default;

		void Update();
		bool IsReadyForSerialization() const;

		const DirectX::ScratchImage& GetFinalOpaqueDiffuseTexture() const;

	private:
		void BeginDiffuseTextureResolution();
		void AddTextureResolutionRenderPasses(D3D12::FrameGraphBuilder& builder);

		void AddSourceTextureUploadRenderPasses(TextureResolutionContext& context);
		void AddBC7CompressionRenderPasses(TextureResolutionContext& context);
		void AddMipMapGenerationRenderPasses(TextureResolutionContext& context);
		void AddReadBackBufferCopyRenderPasses(TextureResolutionContext& context);

		void CopyResolvedTextureToScratchImage();

	private:
		std::optional<ModelTextureResolutionEventHandle> mHDiffuseTextureResolutionEvent;
		std::unique_ptr<D3D12::BufferResource> mOutputBuffer;
		std::vector<std::unique_ptr<BC7ImageCompressor>> mBC7CompressorPtrArr;
		std::vector<D3D12::StructuredBufferSubAllocation<BufferBC7>> mBC7BufferSubAllocationArr;
		DirectX::ScratchImage mDestDiffuseScratchImage;
		DirectX::ScratchImage mSrcDiffuseScratchImage;
		const ImportedMesh* mMeshPtr;
	};
}