module;
#include <vector>
#include <functional>

module Brawler.AssetManagement.AssetDependency;

namespace Brawler
{
	namespace AssetManagement
	{
		void AssetDependency::AddAssetDependencyResolver(std::move_only_function<void(I_AssetIORequestBuilder&)>&& dependencyResolver, const Brawler::JobPriority priority)
		{
			mDependencyResolverArr.emplace_back(std::move(dependencyResolver), priority);
		}
		
		void AssetDependency::MergeAssetDependency(AssetDependency&& dependency)
		{
			mDependencyResolverArr.reserve(mDependencyResolverArr.size() + dependency.mDependencyResolverArr.size());

			for (auto&& resolverInfo : dependency.mDependencyResolverArr)
				mDependencyResolverArr.push_back(std::move(resolverInfo));

			dependency.mDependencyResolverArr.clear();
		}

		void AssetDependency::BuildAssetIORequests(I_AssetIORequestBuilder& requestBuilder)
		{
			for (auto& resolverInfo : mDependencyResolverArr)
			{
				// Set requestBuilder's current priority so that requests made in this call
				// are properly prioritized.
				requestBuilder.SetAssetIORequestPriority(resolverInfo.RequestPriority);

				resolverInfo.ResolverCallback(requestBuilder);
			}
		}
	}
}