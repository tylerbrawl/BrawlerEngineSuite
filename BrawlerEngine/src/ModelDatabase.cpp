module;
#include <cassert>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <unordered_map>

module Brawler.ModelDatabase;
import Brawler.ScopedSharedLock;

namespace Brawler
{
	ModelDatabase& ModelDatabase::GetInstance()
	{
		static ModelDatabase instance{};
		return instance;
	}

	void ModelDatabase::Update()
	{
		const ScopedSharedWriteLock<std::shared_mutex> lock{ mCritSection };

		// Erase all entries which are not being referenced by SceneNodes, i.e.,
		// std::shared_ptr<T>::use_count() returns 1 (implying that the instance
		// owned by the ModelDatabase is the only active one).
		std::erase_if(mModelPathHashMap, [] (const std::pair<const FilePathHash, std::shared_ptr<Model>>& currPair) { return (std::get<1>(currPair).use_count() == 1); });
	}

	ModelHandle ModelDatabase::AddSharedModel(const FilePathHash modelPathHash, Model&& sharedModel)
	{
		std::shared_ptr<Model> sharedModelPtr{ std::make_shared<Model>(std::move(sharedModel)) };
		ModelHandle hModel{ sharedModelPtr };

		{
			const ScopedSharedWriteLock<std::shared_mutex> lock{ mCritSection };

			assert(!mModelPathHashMap.contains(modelPathHash) && "ERROR: An attempt was made to call ModelDatabase::AddSharedModel() for the same FilePathHash value multiple times!");
			mModelPathHashMap.try_emplace(modelPathHash, std::move(sharedModelPtr));
		}

		return hModel;
	}

	std::optional<ModelHandle> ModelDatabase::GetSharedModel(const FilePathHash modelPathHash) const
	{
		std::optional<ModelHandle> hModel{};

		{
			const ScopedSharedReadLock<std::shared_mutex> lock{ mCritSection };

			if (mModelPathHashMap.contains(modelPathHash))
				hModel.emplace(mModelPathHashMap.at(modelPathHash));
		}

		return hModel;
	}
}