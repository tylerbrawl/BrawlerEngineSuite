module;
#include <atomic>
#include <cassert>

module Brawler.SceneTextureHandles;
import Brawler.SceneTextureDatabase;

namespace Brawler
{
	template <typename SceneTextureType>
	SceneTextureHandle<SceneTextureType>::SceneTextureHandle(FilePathHash pathHash) :
		mPathHash(std::move(pathHash))
	{}

	template <typename SceneTextureType>
	SceneTextureHandle<SceneTextureType>::~SceneTextureHandle()
	{
		DecrementReferenceCount();
	}

	template <typename SceneTextureType>
	SceneTextureHandle<SceneTextureType>::SceneTextureHandle(SceneTextureHandle&& rhs) noexcept :
		mPathHash(std::move(rhs.mPathHash))
	{
		rhs.mPathHash.reset();
	}

	template <typename SceneTextureType>
	SceneTextureHandle<SceneTextureType>& SceneTextureHandle<SceneTextureType>::operator=(SceneTextureHandle&& rhs) noexcept
	{
		DecrementReferenceCount();

		mPathHash = std::move(rhs.mPathHash);
		rhs.mPathHash.reset();

		return *this;
	}

	template <typename SceneTextureType>
	std::uint32_t SceneTextureHandle<SceneTextureType>::GetBindlessSRVIndex() const
	{
		assert(IsValid());

		const std::optional<std::uint32_t> bindlessSRVIndex = Brawler::SceneTextureDatabase::GetInstance().GetSceneTextureSRVIndex<SceneTextureType>(*mPathHash);
		assert(bindlessSRVIndex.has_value());

		return *bindlessSRVIndex;
	}

	template <typename SceneTextureType>
	void SceneTextureHandle<SceneTextureType>::DecrementReferenceCount()
	{
		if (IsValid()) [[likely]]
		{
			Brawler::SceneTextureDatabase::GetInstance().DecrementSceneTextureReferenceCount<SceneTextureType>(*mPathHash);

			mPathHash.reset();
		}
	}

	template <typename SceneTextureType>
	bool SceneTextureHandle<SceneTextureType>::IsValid() const
	{
		return mPathHash.has_value();
	}

	template <typename SceneTextureType>
	Brawler::OptionalRef<const SceneTextureType> SceneTextureHandle<SceneTextureType>::GetSceneTexture() const
	{
		assert(IsValid());
		return Brawler::SceneTextureDatabase::GetInstance().GetSceneTexture<SceneTextureType>(*mPathHash);
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template class SceneTextureHandle<SceneTexture2D>;
}