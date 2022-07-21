module;
#include <vector>
#include <filesystem>
#include <span>

export module Brawler.AssetManagement.I_AssetIORequestBuilder;
import Brawler.FilePathHash;
import Brawler.D3D12.I_BufferSubAllocation;
import Brawler.JobPriority;

export namespace Brawler
{
	namespace AssetManagement
	{
		struct CustomFileAssetIORequest
		{
			/// <summary>
			/// Specifies the path to the file which is to be loaded. This does not have to be
			/// the BPK archive, but it can be.
			/// 
			/// To get the path of the BPK archive, call BPKArchiveReader::GetBPKArchiveFilePath().
			/// </summary>
			const std::filesystem::path& FilePath;

			/// <summary>
			/// The offset, in bytes, from the start of the file to begin reading data.
			/// </summary>
			std::size_t FileOffset;

			/// <summary>
			/// The size, in bytes, of the compressed data which must be read. If this value is 0,
			/// then the data is assumed to *NOT* be compressed; otherwise, zstandard is used to
			/// decompress the data.
			/// </summary>
			std::size_t CompressedDataSizeInBytes;

			/// <summary>
			/// The size, in bytess, of the uncompressed data which must be read. The destination
			/// for the request's data *MUST* be large enough to hold this amount of bytes.
			/// </summary>
			std::size_t UncompressedDataSizeInBytes;
		};
	}
}

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
			virtual void AddAssetIORequest(const CustomFileAssetIORequest& customFileRequest, Brawler::D3D12::I_BufferSubAllocation& bufferSubAllocation) = 0;

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