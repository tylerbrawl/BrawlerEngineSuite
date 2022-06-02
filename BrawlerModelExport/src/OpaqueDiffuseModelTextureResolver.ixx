module;
#include <optional>
#include <memory>
#include <DirectXTex.h>

export module Brawler.OpaqueDiffuseModelTextureResolver;
import Brawler.ModelTextureResolutionEventHandle;
import Brawler.ImportedMesh;
import Brawler.D3D12.BufferResource;
import Brawler.D3D12.FrameGraphBuilder;

export namespace Brawler
{
	class OpaqueDiffuseModelTextureResolver
	{
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

	private:
		std::optional<ModelTextureResolutionEventHandle> mHDiffuseTextureResolutionEvent;
		std::unique_ptr<D3D12::BufferResource> mOutputBuffer;
		DirectX::ScratchImage mSrcDiffuseScratchImage;
		ImportedMesh* mMeshPtr;
	};
}