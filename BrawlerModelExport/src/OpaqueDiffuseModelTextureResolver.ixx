module;
#include <optional>
#include <memory>
#include <DirectXTex.h>

export module Brawler.OpaqueDiffuseModelTextureResolver;
import Brawler.ModelTextureResolutionEventHandle;
import Brawler.ImportedMesh;
import Brawler.D3D12.BufferResource;
import Brawler.D3D12.TextureCopyBufferSubAllocation;
import Brawler.D3D12.FrameGraphBuilder;
import Brawler.D3D12.Texture2D;
import Brawler.D3D12.BufferSubAllocationReservationHandle;

export namespace Brawler
{
	class OpaqueDiffuseModelTextureResolver
	{
	private:
		struct TextureResolutionContext
		{
			D3D12::FrameGraphBuilder& Builder;
			D3D12::Texture2D* CurrTexturePtr;
			D3D12::BufferSubAllocationReservationHandle HBC7TextureDataReservation;
		};

	public:
		explicit OpaqueDiffuseModelTextureResolver(const ImportedMesh& mesh);

		OpaqueDiffuseModelTextureResolver(const OpaqueDiffuseModelTextureResolver& rhs) = delete;
		OpaqueDiffuseModelTextureResolver& operator=(const OpaqueDiffuseModelTextureResolver& rhs) = delete;

		OpaqueDiffuseModelTextureResolver(OpaqueDiffuseModelTextureResolver&& rhs) noexcept = default;
		OpaqueDiffuseModelTextureResolver& operator=(OpaqueDiffuseModelTextureResolver&& rhs) noexcept = default;

		void Update();
		bool IsReadyForSerialization() const;

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
		std::vector<D3D12::TextureCopyBufferSubAllocation> mTextureCopySubAllocationArr;
		DirectX::ScratchImage mDestDiffuseScratchImage;
		DirectX::ScratchImage mSrcDiffuseScratchImage;
		const ImportedMesh* mMeshPtr;
	};
}