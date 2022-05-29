module;
#include <unordered_map>
#include <filesystem>
#include <atomic>
#include <span>

export module Brawler.BCAInfoDatabase;
import Brawler.BCAInfo;

export namespace Brawler
{
	class BCAInfoDatabase final
	{
	private:
		struct DatabaseSubMap
		{
			std::unordered_map<std::filesystem::path, BCAInfo> AssetPathMap;
			std::atomic<bool> ReadyFlag;
		};

	private:
		BCAInfoDatabase() = default;

	public:
		~BCAInfoDatabase() = default;

		BCAInfoDatabase(const BCAInfoDatabase& rhs) = delete;
		BCAInfoDatabase& operator=(const BCAInfoDatabase& rhs) = delete;

		BCAInfoDatabase(BCAInfoDatabase&& rhs) noexcept = delete;
		BCAInfoDatabase& operator=(BCAInfoDatabase&& rhs) noexcept = delete;

		static BCAInfoDatabase& GetInstance();

		void RegisterBCAInfoFilePath(const std::filesystem::path& bcaInfoFilePath);
		void UpdateBCAInfoForSourceAssets(const std::span<std::pair<const std::filesystem::path&, BCAInfo>> assetBCAInfoPairSpan);

		const BCAInfo& GetBCAInfoForSourceAsset(const std::filesystem::path& assetFilePath) const;

	private:
		std::unordered_map<std::filesystem::path, DatabaseSubMap> mBCAInfoParentPathMap;
	};
}