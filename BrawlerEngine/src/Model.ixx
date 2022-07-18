module;
#include <memory>
#include <vector>

export module Brawler.Model;
import Brawler.I_LODMeshDefinition;
import Brawler.FilePathHash;

export namespace Brawler
{
	class Model
	{
	public:
		Model() = default;
		explicit Model(const FilePathHash bmdlFileHash);

		Model(const Model& rhs) = delete;
		Model& operator=(const Model& rhs) = delete;

		Model(Model&& rhs) noexcept = default;
		Model& operator=(Model&& rhs) noexcept = default;

	private:
		std::vector<std::unique_ptr<I_LODMeshDefinition>> mLODMeshDefinitionPtrArr;
	};
}