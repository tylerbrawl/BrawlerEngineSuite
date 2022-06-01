module;
#include <optional>

export module Brawler.OpaqueDiffuseModelTextureResolver;
import Brawler.ModelTextureResolutionEventHandle;
import Brawler.ImportedMesh;

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

	private:
		std::optional<ModelTextureResolutionEventHandle> mHDiffuseTextureResolution;
		ImportedMesh* mMeshPtr;
	};
}