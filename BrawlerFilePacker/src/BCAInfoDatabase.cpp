module;
#include <filesystem>
#include <unordered_map>
#include <atomic>
#include <span>
#include <cassert>
#include <ranges>

module Brawler.BCAInfoDatabase;
import Util.Coroutine;

namespace Brawler
{
	BCAInfoDatabase& BCAInfoDatabase::GetInstance()
	{
		static BCAInfoDatabase instance{};
		return instance;
	}

	void BCAInfoDatabase::RegisterBCAInfoFilePath(const std::filesystem::path& bcaInfoFilePath)
	{
		// We want to use the hash of the parent directory of the .BCAINFO file. That way, we
		// can easily index mBCAInfoParentPathMap using the parent directories of asset file
		// paths from BCAArchive instances.
		assert(std::filesystem::equivalent(bcaInfoFilePath.filename(), BCA_INFO_FILE_PATH) && !std::filesystem::is_directory(bcaInfoFilePath) && "ERROR: A file path which did not refer to a .BCAINFO file was provided to BCAInfoDatabase::RegisterBCAInfoFilePath()!");

		mBCAInfoParentPathMap.try_emplace(bcaInfoFilePath.parent_path());
	}

	void BCAInfoDatabase::UpdateBCAInfoForSourceAssets(const std::span<std::pair<std::filesystem::path, BCAInfo>> assetBCAInfoPairSpan)
	{
		if (assetBCAInfoPairSpan.empty()) [[unlikely]]
			return;

		const std::filesystem::path parentDir{ std::get<0>(assetBCAInfoPairSpan[0]).parent_path() };

#ifdef _DEBUG
		// Make sure that all of the supplied asset file paths have the same parent directory.
		{
			const std::filesystem::path parentDir{ std::get<0>(assetBCAInfoPairSpan[0]).parent_path() };

			for (const auto& assetBCAInfoPair : assetBCAInfoPairSpan | std::views::drop(1))
			{
				assert(std::get<0>(assetBCAInfoPair).parent_path() == parentDir);
			}
		}
#endif // _DEBUG

		assert(mBCAInfoParentPathMap.contains(parentDir));

		DatabaseSubMap& subMap{ mBCAInfoParentPathMap.at(parentDir) };

		for (auto& assetBCAInfoPair : assetBCAInfoPairSpan)
			subMap.AssetPathMap.try_emplace(std::get<0>(assetBCAInfoPair), std::move(std::get<1>(assetBCAInfoPair)));

		// Let other threads access the BCAInfo data.
		subMap.ReadyFlag.store(true, std::memory_order::release);
	}

	const BCAInfo& BCAInfoDatabase::GetBCAInfoForSourceAsset(const std::filesystem::path& assetFilePath) const
	{
		assert(std::filesystem::absolute(assetFilePath) == assetFilePath);
		
		const std::filesystem::path parentDir{ assetFilePath.parent_path() };

		// Since BCAInfoDatabase::RegisterBCAInfoFilePath() is called for all relevant .BCAINFO
		// files before any calls to BCAInfoDatabase::GetBCAInfoForSourceAsset() are made, we
		// know that if mBCAInfoParentPathMap does not contain an entry for the asset's parent
		// directory, then no .BCAINFO file exists in that directory. Thus, we can just return
		// the default BCAInfo value in that case.

		if (!mBCAInfoParentPathMap.contains(parentDir))
			return DEFAULT_BCA_INFO_VALUE;

		const DatabaseSubMap& subMap{ mBCAInfoParentPathMap.at(parentDir) };

		// Go do something else while we wait for the BCAInfo data to be generated concurrently.
		while (!subMap.ReadyFlag.load(std::memory_order::acquire))
			Util::Coroutine::TryExecuteJob();

		return subMap.AssetPathMap.at(assetFilePath);
	}
}