module;
#include <vector>

export module Brawler.AssetManagement.I_AssetIORequestBuilder;
import Brawler.FilePathHash;
import Brawler.D3D12.I_BufferSubAllocation;
import Brawler.JobPriority;

export namespace Brawler
{
	namespace AssetManagement
	{
		class I_AssetIORequestBuilder
		{
		protected:
			I_AssetIORequestBuilder() = default;

		public:
			virtual ~I_AssetIORequestBuilder() = default;

			I_AssetIORequestBuilder(const I_AssetIORequestBuilder& rhs) = delete;
			I_AssetIORequestBuilder& operator=(const I_AssetIORequestBuilder& rhs) = delete;

			I_AssetIORequestBuilder(I_AssetIORequestBuilder&& rhs) noexcept = default;
			I_AssetIORequestBuilder& operator=(I_AssetIORequestBuilder&& rhs) noexcept = default;

			virtual void AddAssetIORequest(const Brawler::FilePathHash pashHash, Brawler::D3D12::I_BufferSubAllocation& bufferSubAllocation) = 0;

			/// <summary>
			/// Sets the priority for requests made in subsequent calls to
			/// I_AssetIORequestBuilder::AddAssetIORequest(). By default, the priority will be set
			/// to that which was specified when passing an asset dependency resolver callback to
			/// AssetDependency::AddAssetDependencyResolver(). (If no priority is specified, the
			/// default priority is Brawler::JobPriority::NORMAL.)
			/// 
			/// There is no need to call this function if it is acceptable to use the priority
			/// specified in the call to the aforementioned function. However, changing the priority
			/// here allows for more fine-grained priority setting, if it is required.
			/// </summary>
			/// <param name="priority">
			/// - The Brawler::JobPriority describing the priority of requests made in subsequent
			///   calls to I_AssetIORequestBuilder::AddAssetIORequest().
			/// </param>
			void SetAssetIORequestPriority(const Brawler::JobPriority priority);

			Brawler::JobPriority GetAssetIORequestPriority() const;

		private:
			Brawler::JobPriority mPriority;
		};
	}
}