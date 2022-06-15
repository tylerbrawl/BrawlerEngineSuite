module;
#include <vector>
#include <functional>

export module Brawler.AssetManagement.AssetDependency;
import Brawler.AssetManagement.I_AssetIORequestBuilder;
import Brawler.JobPriority;

export namespace Brawler
{
	namespace AssetManagement
	{
		class I_AssetResolver;
	}
}

export namespace Brawler
{
	namespace AssetManagement
	{
		class AssetDependency
		{
		private:
			struct DependencyResolverInfo
			{
				std::move_only_function<void(I_AssetIORequestBuilder&)> ResolverCallback;
				Brawler::JobPriority RequestPriority;
			};

		public:
			AssetDependency() = default;

			AssetDependency(const AssetDependency& rhs) = delete;
			AssetDependency& operator=(const AssetDependency& rhs) = delete;

			AssetDependency(AssetDependency&& rhs) noexcept = default;
			AssetDependency& operator=(AssetDependency&& rhs) noexcept = default;

			/// <summary>
			/// Adds the callback specified by dependencyResolver as an asset dependency resolver.
			/// This callback must have the function signature void(I_AssetIORequestBuilder&).
			/// 
			/// Eventually, the callback will be called with an I_AssetIORequestBuilder instance.
			/// The idea is that the callback will tell the builder which requests need to be
			/// fulfilled in order to consider the asset dependency to be resolved.
			/// 
			/// For instance, consider a buffer sub-allocation which is to be used as the vertex
			/// buffer for a mesh. The dependency resolver callback would inform the I_AssetIORequestBuilder
			/// that the data found within the BPK archive at FilePathHash X contains the data which
			/// must be written into the buffer sub-allocation.
			/// 
			/// *NOTE*: The caller is responsible for ensuring that any objects specified to be written
			/// into via a dependency resolver callback are kept alive until the AssetRequestEventHandle
			/// returned by calling AssetManager::EnqueueAssetDependency() signals that the request has
			/// been completed. This can be checked by calling 
			/// AssetRequestEventHandle::IsAssetRequestComplete().
			/// </summary>
			/// <param name="dependencyResolver">
			/// - The callback which should specify describe the required asset I/O requests to its
			///   provided I_AssetIORequestBuilder&. See the summary for more details.
			/// </param>
			/// <param name="priority">
			/// - The priority of requests made in a call to dependencyResolver. The asset I/O request handlers
			///   use this to determine the order in which requests must be fulfilled.
			/// </param>
			void AddAssetDependencyResolver(std::move_only_function<void(I_AssetIORequestBuilder&)>&& dependencyResolver, const Brawler::JobPriority priority = Brawler::JobPriority::NORMAL);

			/// <summary>
			/// Merges the I_AssetResolver instances contained within dependency into this
			/// AssetDependency instance.
			/// 
			/// The order in which I_AssetResolver instances are completed is *UNDEFINED*,
			/// regardless of whether they were merged into an AssetDependency instance or created
			/// directly within one. This is a natural consequence of the multithreaded nature
			/// of asset loading.
			/// 
			/// To be perfectly correct, it would be possible to establish some form of dependency
			/// ordering between I_AssetResolver instances, even if assets are loaded concurrently.
			/// However, we deem this to be a poor design choice, since it both adds complexity and
			/// limits parallelism. If a set of steps must be executed sequentially, then they should
			/// all be done within the same I_AssetResolver instance.
			/// 
			/// If this type of behavior really is needed, then one solution might be to create
			/// and submit AssetDependency instances from within I_AssetResolver instances, once they
			/// are ready.
			/// </summary>
			/// <param name="dependency">
			/// - The AssetDependency instance whose I_AssetResolver instances are to be merged into
			///   this AssetDependency instance.
			/// </param>
			void MergeAssetDependency(AssetDependency&& dependency);

			void BuildAssetIORequests(I_AssetIORequestBuilder& requestBuilder);

		private:
			std::vector<DependencyResolverInfo> mDependencyResolverArr;
		};
	}
}