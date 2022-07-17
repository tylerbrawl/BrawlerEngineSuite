module;
#include <span>
#include <vector>
#include <cassert>
#include <zstd.h>
#include "DxDef.h"

module Brawler.ZSTDCompressionOperation;
import Util.ZSTD;

namespace Brawler
{
	HRESULT ZSTDCompressionOperation::BeginCompressionOperation(const std::span<const std::byte> srcDataSpan)
	{
		if (srcDataSpan.empty()) [[unlikely]]
			return E_INVALIDARG;

		const std::size_t zstdError = ZSTD_CCtx_reset(mCompressionContext.Get(), ZSTD_ResetDirective::ZSTD_reset_session_and_parameters);

		if (ZSTD_isError(zstdError)) [[unlikely]]
			return Util::ZSTD::ZSTDErrorToHRESULT(zstdError);

		mInputBuffer = ZSTD_inBuffer{
			.src = srcDataSpan.data(),
			.size = srcDataSpan.size_bytes(),
			.pos = 0
		};
		mSrcDataSpan = srcDataSpan;
		mOperationCompleted = false;

		return S_OK;
	}

	HRESULT ZSTDCompressionOperation::ContinueCompressionOperation(const std::span<std::byte> destDataSpan)
	{
		if (destDataSpan.empty() || IsCompressionComplete()) [[unlikely]]
			return S_OK;

		ZSTD_outBuffer outputBuffer{
			.dst = destDataSpan.data(),
			.size = destDataSpan.size_bytes(),
			.pos = 0
		};

		const std::size_t zstdError = ZSTD_compressStream2(
			mCompressionContext.Get(),
			&outputBuffer,
			&mInputBuffer,
			ZSTD_EndDirective::ZSTD_e_continue
		);

		if (mInputBuffer.pos == mInputBuffer.size)
			mOperationCompleted = true;

		if (ZSTD_isError(zstdError)) [[unlikely]]
			return Util::ZSTD::ZSTDErrorToHRESULT(zstdError);

		return S_OK;
	}

	ZSTDCompressionOperation::CompressionResults ZSTDCompressionOperation::FinishCompressionOperation()
	{
		if (IsCompressionComplete()) [[unlikely]]
		{
			return CompressionResults{
				.CompressedByteArr{},
				.HResult = S_OK
			};
		}

		using Block = std::vector<std::byte>;

		std::vector<Block> blockArr{};
		std::size_t numBytesCompressed = 0;

		while (!IsCompressionComplete())
		{
			Block currBlock{};
			currBlock.resize(GetZSTDBlockSize());

			ZSTD_outBuffer outputBuffer{
				.dst = currBlock.data(),
				.size = currBlock.size(),
				.pos = 0
			};

			assert(outputBuffer.size > 0);

			const std::size_t zstdError = ZSTD_compressStream2(
				mCompressionContext.Get(),
				&outputBuffer,
				&mInputBuffer,
				ZSTD_EndDirective::ZSTD_e_continue
			);

			if (ZSTD_isError(zstdError)) [[unlikely]]
			{
				return CompressionResults{
					.CompressedByteArr{},
					.HResult = Util::ZSTD::ZSTDErrorToHRESULT(zstdError)
				};
			}

			numBytesCompressed += outputBuffer.pos;

			if (mInputBuffer.pos == mInputBuffer.size)
			{
				if (outputBuffer.pos != outputBuffer.size) [[likely]]
				{
					currBlock.resize(outputBuffer.pos);
					currBlock.shrink_to_fit();
				}

				mOperationCompleted = true;
			}

			blockArr.push_back(std::move(currBlock));
		}

		// Move all of the compressed bytes into a single contiguous block of
		// memory.
		std::vector<std::byte> compressedByteArr{};
		compressedByteArr.resize(numBytesCompressed);

		std::span<std::byte> copyDestSpan{ compressedByteArr };

		for (const auto& block : blockArr)
		{
			std::memcpy(copyDestSpan.data(), block.data(), block.size());
			copyDestSpan = copyDestSpan.subspan(block.size());
		}

		return CompressionResults{
			.CompressedByteArr{ std::move(compressedByteArr) },
			.HResult = S_OK
		};
	}

	HRESULT ZSTDCompressionOperation::FinishCompressionOperation(const std::span<std::byte> destDataSpan)
	{
		const HRESULT hr = ContinueCompressionOperation(destDataSpan);

		if (FAILED(hr)) [[unlikely]]
			return hr;

		return (IsCompressionComplete() ? S_OK : E_NOT_SUFFICIENT_BUFFER);
	}

	bool ZSTDCompressionOperation::IsCompressionComplete() const
	{
		return mOperationCompleted;
	}

	std::size_t ZSTDCompressionOperation::GetZSTDBlockSize() const
	{
		return ZSTD_CStreamOutSize();
	}
}