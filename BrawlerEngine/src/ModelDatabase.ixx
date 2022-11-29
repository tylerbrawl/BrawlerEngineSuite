module;
#include <memory>
#include <optional>
#include <shared_mutex>
#include <unordered_map>

export module Brawler.ModelDatabase;
import Brawler.Model;
import Brawler.ModelHandle;
import Brawler.FilePathHash;

export namespace Brawler 
{
	class ModelDatabase final
	{
	private:
		ModelDatabase() = default;

	public:
		~ModelDatabase() = default;

		ModelDatabase(const ModelDatabase& rhs) = delete;
		ModelDatabase& operator=(const ModelDatabase& rhs) = delete;

		ModelDatabase(ModelDatabase&& rhs) noexcept = delete;
		ModelDatabase& operator=(ModelDatabase&& rhs) noexcept = delete;

		static ModelDatabase& GetInstance();

		void Update();

		ModelHandle AddSharedModel(const FilePathHash modelPathHash, Model&& sharedModel);
		std::optional<ModelHandle> GetSharedModel(const FilePathHash modelPathHash) const;

	private:
		std::unordered_map<FilePathHash, std::shared_ptr<Model>> mModelPathHashMap;
		mutable std::shared_mutex mCritSection;
	};
}