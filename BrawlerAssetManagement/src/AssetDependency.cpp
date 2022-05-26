module;
#include <memory>
#include <vector>

module Brawler.AssetManagement.AssetDependency;

namespace Brawler
{
	namespace AssetManagement
	{
		void AssetDependency::MergeAssetDependency(AssetDependency&& dependency)
		{
			mAssetLoaderDependencyArr.reserve(mAssetLoaderDependencyArr.size() + dependency.mAssetLoaderDependencyArr.size());

			for (auto&& assetLoaderPtr : dependency.mAssetLoaderDependencyArr)
				mAssetLoaderDependencyArr.push_back(std::move(assetLoaderPtr));

			dependency.mAssetLoaderDependencyArr.clear();
		}
	}
}