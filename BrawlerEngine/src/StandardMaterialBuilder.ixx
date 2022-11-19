module;
#include <optional>

export module Brawler.StandardMaterialDefinition:StandardMaterialBuilder;
import Brawler.FilePathHash;

export namespace Brawler
{
	class StandardMaterialBuilder
	{
	public:
		StandardMaterialBuilder() = default;

		StandardMaterialBuilder(const StandardMaterialBuilder& rhs) = delete;
		StandardMaterialBuilder& operator=(const StandardMaterialBuilder& rhs) = delete;

		StandardMaterialBuilder(StandardMaterialBuilder&& rhs) noexcept = delete;
		StandardMaterialBuilder& operator=(StandardMaterialBuilder&& rhs) noexcept = delete;

		void SetBaseColorFilePathHash(const FilePathHash pathHash);
		std::optional<FilePathHash> GetBaseColorFilePathHash() const;

		void SetNormalMapFilePathHash(const FilePathHash pathHash);
		std::optional<FilePathHash> GetNormalMapFilePathHash() const;

		void SetRoughnessFilePathHash(const FilePathHash pathHash);
		std::optional<FilePathHash> GetRoughnessFilePathHash() const;

		void SetMetallicFilePathHash(const FilePathHash pathHash);
		std::optional<FilePathHash> GetMetallicFilePathHash() const;

	private:
		std::optional<FilePathHash> mBaseColorTextureHash;
		std::optional<FilePathHash> mNormalMapTextureHash;
		std::optional<FilePathHash> mRoughnessTextureHash;
		std::optional<FilePathHash> mMetallicTextureHash;
	};
}