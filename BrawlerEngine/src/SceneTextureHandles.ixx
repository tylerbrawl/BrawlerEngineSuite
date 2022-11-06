module;
#include <atomic>
#include <cassert>
#include <optional>

export module Brawler.SceneTextureHandles;
import Brawler.SceneTextures;
import Brawler.FilePathHash;
import Brawler.OptionalRef;

export namespace Brawler
{
	template <typename SceneTextureType>
	class SceneTextureHandle
	{
	public:
		SceneTextureHandle() = default;
		explicit SceneTextureHandle(FilePathHash pathHash);

		~SceneTextureHandle();

		SceneTextureHandle(const SceneTextureHandle& rhs) = delete;
		SceneTextureHandle& operator=(const SceneTextureHandle& rhs) = delete;

		SceneTextureHandle(SceneTextureHandle&& rhs) noexcept;
		SceneTextureHandle& operator=(SceneTextureHandle&& rhs) noexcept;

		const SceneTextureType& operator*() const;
		const SceneTextureType* operator->() const;

	private:
		void DecrementReferenceCount();

		bool IsValid() const;
		Brawler::OptionalRef<const SceneTextureType> GetSceneTexture() const;

	private:
		std::optional<FilePathHash> mPathHash;
	};
}

// -----------------------------------------------------------------------------------------------------------------------------------------

export namespace Brawler
{
	using SceneTexture2DHandle = SceneTextureHandle<SceneTexture2D>;
}