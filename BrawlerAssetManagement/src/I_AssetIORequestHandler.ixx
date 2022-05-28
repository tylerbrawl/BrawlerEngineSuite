module;
#include <cstddef>
#include <memory>

export module Brawler.AssetManagement.I_AssetIORequestHandler;
import Brawler.AssetManagement.EnqueuedAssetDependency;

export namespace Brawler
{
	namespace AssetManagement
	{
		class I_AssetIORequestHandler
		{
		protected:
			I_AssetIORequestHandler() = default;

		public:
			virtual ~I_AssetIORequestHandler() = default;

			I_AssetIORequestHandler(const I_AssetIORequestHandler& rhs) = delete;
			I_AssetIORequestHandler& operator=(const I_AssetIORequestHandler& rhs) = delete;

			I_AssetIORequestHandler(I_AssetIORequestHandler&& rhs) noexcept = default;
			I_AssetIORequestHandler& operator=(I_AssetIORequestHandler&& rhs) noexcept = default;

			/// <summary>
			/// Asset I/O requests are handled in a two-step process:
			/// 
			///   1. Asset dependency resolver callbacks which were specified in an AssetDependency instance
			///      are called with an I_AssetIORequestBuilder instance constructed by the current 
			///		 I_AssetIORequestHandler instance. The request handler uses the builder to construct API-specific 
			///		 asset load requests and prepares them for execution.
			/// 
			///   2. Asset data loading is handled by a selection of threads. Once all of the asset data for
			///      an AssetDependency instance has been loaded, the corresponding AssetRequestEventHandle
			///      instance is signalled.
			/// 
			/// Derived classes should fulfill Step 1's requirements in their implementation of
			/// I_AssetIORequestHandler::PrepareAssetIORequest().
			/// 
			/// *NOTE*: The implementation *MUST* be thread safe.
			/// </summary>
			/// <param name="enqueuedDependency">
			/// - A std::unique_ptr&lt;EnqueuedAssetDependency&gt; instance which contains both the AssetDependency
			///   to be resolved and the corresponding AssetRequestEventHandle. Derived classes will need to cache
			///   at least the AssetRequestEventHandle instance for Step 2.
			/// </param>
			virtual void PrepareAssetIORequest(std::unique_ptr<EnqueuedAssetDependency>&& enqueuedDependency) = 0;

			/// <summary>
			/// Asset I/O requests are handled in a two-step process:
			/// 
			///   1. Asset dependency resolver callbacks which were specified in an AssetDependency instance
			///      are called with an I_AssetIORequestBuilder instance constructed by the current 
			///		 I_AssetIORequestHandler instance. The request handler uses the builder to construct API-specific 
			///		 asset load requests and prepares them for execution.
			/// 
			///   2. Asset data loading is handled by the current I_AssetIORequestHandler. Once all of the asset data for
			///      an AssetDependency instance has been loaded, the corresponding AssetRequestEventHandle
			///      instance is signalled.
			/// 
			/// Derived classes should fulfill Step 2's requirements in their implementation of
			/// I_AssetIORequestHandler::SubmitAssetIORequests().
			/// 
			/// This function is called on a single thread because it is impossible for the AssetManager to determine
			/// how many threads the I_AssetIORequestHandler instance will need. However, the expectation is that
			/// said handler instance will create a number of CPU jobs corresponding to the current AssetLoadingMode.
			/// 
			/// Creating CPU jobs during this function may be unnecessary, though. For instance, the DirectStorage
			/// API handles I/O requests on separate threads, so the DirectStorageAssetIORequestHandler only needs
			/// a single thread during this function.
			/// </summary>
			virtual void SubmitAssetIORequests() = 0;
		};
	}
}