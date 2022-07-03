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
import Brawler.BC7ImageCompressor;
import Brawler.VirtualTexturePage;
import Brawler.VirtualTextureCPUDataStore;
import Brawler.TextureTypeMap;
import Brawler.ModelTextureID;
import Brawler.FilePathHash;

export namespace Brawler
{
	class OpaqueDiffuseModelTextureResolver
	{
	private:
		struct TextureResolutionContext
		{
			D3D12::FrameGraphBuilder& Builder;
			D3D12::Texture2D* CurrTexturePtr;
			std::vector<VirtualTexturePage> VirtualTexturePageArr;
		};

	public:
		explicit OpaqueDiffuseModelTextureResolver(const ImportedMesh& mesh);

		OpaqueDiffuseModelTextureResolver(const OpaqueDiffuseModelTextureResolver& rhs) = delete;
		OpaqueDiffuseModelTextureResolver& operator=(const OpaqueDiffuseModelTextureResolver& rhs) = delete;

		OpaqueDiffuseModelTextureResolver(OpaqueDiffuseModelTextureResolver&& rhs) noexcept = default;
		OpaqueDiffuseModelTextureResolver& operator=(OpaqueDiffuseModelTextureResolver&& rhs) noexcept = default;

		void Update();
		bool IsReadyForSerialization() const;

		FilePathHash SerializeModelTexture() const;

	private:
		void BeginDiffuseTextureResolution();
		void AddTextureResolutionRenderPasses(D3D12::FrameGraphBuilder& builder);

		void AddSourceTextureUploadRenderPasses(TextureResolutionContext& context);
		void AddMipMapGenerationRenderPasses(TextureResolutionContext& context);
		void AddVirtualTexturePartitioningRenderPasses(TextureResolutionContext& context);
		void AddBC7CompressionRenderPasses(TextureResolutionContext& context);
		void AddCPUDataStoreCopyRenderPasses(TextureResolutionContext& context);

	private:
		std::optional<ModelTextureResolutionEventHandle> mHDiffuseTextureResolutionEvent;
		AnisotropicFilterVirtualTextureCPUDataStore<Brawler::GetDesiredTextureFormat<ModelTextureID::DIFFUSE_ALBEDO>()> mTextureDataStore;
		std::vector<std::unique_ptr<BC7ImageCompressor>> mBC7CompressorPtrArr;
		DirectX::ScratchImage mSrcDiffuseScratchImage;
		const ImportedMesh* mMeshPtr;
	};
}