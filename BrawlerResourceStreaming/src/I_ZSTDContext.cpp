module;
#include <vector>
#include <span>
#include <string>
#include <cassert>
#include <stdexcept>
#include <zstd.h>

module Brawler.I_ZSTDContext;

namespace Brawler
{
	I_ZSTDContext::I_ZSTDContext() :
		mContextPtr(ZSTD_createDCtx())
	{}

	I_ZSTDContext::~I_ZSTDContext()
	{
		DeleteDecompressionContext();
	}

	I_ZSTDContext::I_ZSTDContext(I_ZSTDContext&& rhs) noexcept :
		mContextPtr(rhs.mContextPtr)
	{
		rhs.mContextPtr = nullptr;
	}

	I_ZSTDContext& I_ZSTDContext::operator=(I_ZSTDContext&& rhs) noexcept
	{
		DeleteDecompressionContext();

		mContextPtr = rhs.mContextPtr;
		rhs.mContextPtr = nullptr;

		return *this;
	}

	std::vector<std::uint8_t> I_ZSTDContext::DecompressData(const std::size_t numBlocks)
	{
		std::size_t outputSize = GetBlockSize(numBlocks);
		std::vector<std::uint8_t> outputBuffer{};
		outputBuffer.resize(outputSize);

		ZSTD_outBuffer zOutBuffer{
			.dst = reinterpret_cast<void*>(outputBuffer.data()),
			.size = outputSize,
			.pos = 0
		};

		std::size_t numBlocksDecompressed = 0;

		// Decompress until we either run out of blocks to decompress or have already decompressed the
		// requested number of ZSTD blocks, whichever comes first.
		while (!IsInputFinished() && (numBlocksDecompressed < numBlocks))
		{
			const std::span<const std::uint8_t> inputSpan{ FetchInputData(ZSTD_DStreamInSize()) };
			ZSTD_inBuffer zInBuffer{
				.src = reinterpret_cast<const void*>(inputSpan.data()),
				.size = inputSpan.size_bytes(),
				.pos = 0
			};

			// Decompress the entirety of the current ZSTD block.
			while (zInBuffer.pos < zInBuffer.size)
			{
				// Usually, GetBlockSize(numBlocks) returns a sufficient size for an output buffer,
				// because it internally uses ZSTD_DStreamOutSize(). However, it is still possible
				// that this is not enough, although this case is somewhat rare.
				//
				// I'm unsure if this is a bug in ZSTD or a misunderstanding on my part.
				if (zOutBuffer.pos == zOutBuffer.size) [[unlikely]]
				{
					outputSize += GetBlockSize();
					outputBuffer.resize(outputSize);

					// Calling std::vector::resize() requires an allocation on the heap of a new
					// contiguous block of memory. This means that we have to replace the old
					// destination address with this newly allocated block.
					zOutBuffer.dst = reinterpret_cast<void*>(outputBuffer.data());
					zOutBuffer.size = outputSize;
				}

				const std::size_t inputBufferSize = ZSTD_decompressStream(mContextPtr, &zOutBuffer, &zInBuffer);

				// If inputBufferSize == 0, then ZSTD is telling us that we have finished
				// decompressing the entire frame. However, what if we want to loop the
				// decompression, such as for background music? This is why we rely on the
				// IsInputFinished() function to determine when to stop decompressing data.
				//
				// So, if we find that ZSTD_decompressStream() returns 0 (i.e., we have finished
				// decompressing the frame), we need to reset the decompression context.

				if (inputBufferSize == 0)
					ResetDecompressionContext();
			}

			++numBlocksDecompressed;
		}

		// I get the feeling that ZSTD_DStreamOutSize() is an upper bound on the actual required
		// output buffer size, and that most of the time, we don't need that much space. So, in
		// the event that we did not use the entire buffer, we will shrink it down.
		//
		// This is important, since other APIs probably won't handle extra zeroes after their 
		// expected data nicely.
		if (zOutBuffer.pos < zOutBuffer.size) [[likely]]
		{
			outputBuffer.resize(zOutBuffer.pos);
			outputBuffer.shrink_to_fit();
		}

		return outputBuffer;
	}

	std::size_t I_ZSTDContext::GetBlockSize(const std::size_t numBlocks) const
	{
		return (ZSTD_DStreamOutSize() * numBlocks);
	}

	void I_ZSTDContext::ResetDecompressionContext()
	{
		const std::size_t errorCode = ZSTD_DCtx_reset(mContextPtr, ZSTD_reset_session_only);

		if (ZSTD_isError(errorCode)) [[unlikely]]
			throw std::runtime_error{ std::string{"ERROR: The following error occurred when attempting to reset the ZSTD decompression context: "} + ZSTD_getErrorName(errorCode) };
	}

	void I_ZSTDContext::DeleteDecompressionContext()
	{
		if (mContextPtr != nullptr) [[likely]]
		{
			ZSTD_freeDCtx(mContextPtr);
			mContextPtr = nullptr;
		}
	}
}