module;
#include <vector>

export module Brawler.AssetManager.I_AssetIORequestBuilder;
import Brawler.FilePathHash;
import Brawler.D3D12.I_BufferSubAllocation;

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
		};
	}
}