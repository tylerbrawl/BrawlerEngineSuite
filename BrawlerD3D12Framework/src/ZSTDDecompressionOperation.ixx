module;
#include <span>
#include <vector>
#include <zstd.h>
#include "DxDef.h"

export module Brawler.ZSTDDecompressionOperation;
import Brawler.ZSTDContext;

export namespace Brawler
{
	class ZSTDDecompressionOperation
	{
	public:
		struct DecompressionResults
		{
			std::vector<std::byte> DecompressedByteArr;
			HRESULT HResult;
		};

	public:
		ZSTDDecompressionOperation() = default;

		ZSTDDecompressionOperation(const ZSTDDecompressionOperation& rhs) = delete;
		ZSTDDecompressionOperation& operator=(const ZSTDDecompressionOperation& rhs) = delete;

		ZSTDDecompressionOperation(ZSTDDecompressionOperation&& rhs) noexcept = default;
		ZSTDDecompressionOperation& operator=(ZSTDDecompressionOperation&& rhs) noexcept = default;

		HRESULT BeginDecompressionOperation(std::span<const std::byte> srcDataSpan);
		HRESULT ContinueDecompressionOperation(const std::span<std::byte> destDataSpan);

		DecompressionResults FinishDecompressionOperation();
		HRESULT FinishDecompressionOperation(const std::span<std::byte> destDataSpan);

		bool IsDecompressionComplete() const;

		std::size_t GetZSTDBlockSize() const;

	private:
		ZSTDDecompressionContext mDecompressionContext;
		std::span<const std::byte> mSrcDataSpan;
		ZSTD_inBuffer mInputBuffer;
		bool mOperationFinished;
	};
}