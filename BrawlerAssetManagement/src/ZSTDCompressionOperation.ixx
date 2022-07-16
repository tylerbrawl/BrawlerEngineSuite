module;
#include <span>
#include <zstd.h>
#include <DxDef.h>

export module Brawler.AssetManagement.ZSTDCompressionOperation;
import Brawler.AssetManagement.ZSTDContext;

export namespace Brawler
{
	namespace AssetManagement
	{
		class ZSTDCompressionOperation
		{
		public:
			struct CompressionResults
			{
				std::vector<std::byte> CompressedByteArr;
				HRESULT HResult;
			};

		public:
			ZSTDCompressionOperation() = default;

			ZSTDCompressionOperation(const ZSTDCompressionOperation& rhs) = delete;
			ZSTDCompressionOperation& operator=(const ZSTDCompressionOperation& rhs) = delete;

			ZSTDCompressionOperation(ZSTDCompressionOperation&& rhs) noexcept = default;
			ZSTDCompressionOperation& operator=(ZSTDCompressionOperation&& rhs) noexcept = default;

			HRESULT BeginCompressionOperation(const std::span<const std::byte> srcDataSpan);
			HRESULT ContinueCompressionOperation(const std::span<std::byte> destDataSpan);

			CompressionResults FinishCompressionOperation();
			HRESULT FinishCompressionOperation(const std::span<std::byte> destDataSpan);

			bool IsCompressionComplete() const;
			
			std::size_t GetZSTDBlockSize() const;

		private:
			ZSTDCompressionContext mCompressionContext;
			ZSTD_inBuffer mInputBuffer;
			std::span<const std::byte> mSrcDataSpan;
			bool mOperationCompleted;
		};
	}
}