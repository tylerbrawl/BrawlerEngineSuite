module;
#include <vector>
#include <algorithm>
#include <cassert>
#include <span>
#include <zstd.h>

module Brawler.BPKZSTDContext;
import Util.BPK;
import Brawler.BPKArchiveReader;
import Brawler.FileMapper;

namespace
{
	static const std::size_t ZSTD_INPUT_CHUNK_SIZE = ZSTD_DStreamInSize();
}

namespace IMPL
{
	ZSTDChunkRingBuffer::ZSTDChunkRingBuffer(const Brawler::FilePathHash& pathHash, bool allowWrapAround) :
		mRingBuffer(),
		mCurrIndex(0),
		mAllowWrapAround(allowWrapAround)
	{
		InitializeRingBuffer(pathHash);
	}

	void ZSTDChunkRingBuffer::InitializeRingBuffer(const Brawler::FilePathHash& pathHash)
	{
		const Brawler::FileMapper& bpkMapper{ Util::BPK::GetBPKArchiveReader().GetBPKFileMapper() };
		const Brawler::BPKArchiveReader::TOCEntry& assetTOCEntry{ Util::BPK::GetBPKArchiveReader().GetTableOfContentsEntry(pathHash) };

		// Get the total number of ZSTD input chunks from the size of the ZSTD archive.
		const std::size_t chunkCount = ((assetTOCEntry.CompressedSizeInBytes / ZSTD_INPUT_CHUNK_SIZE) + 1);
		mRingBuffer.reserve(chunkCount);

		for (std::size_t i = 0; i < chunkCount; ++i)
		{
			// The number of bytes which have not been reserved for a ZSTDChunk equals the size of
			// the archive minus the size of all of the ZSTDChunks which have already been reserved.
			const std::size_t bytesAvailableForChunk = (assetTOCEntry.CompressedSizeInBytes - (ZSTD_INPUT_CHUNK_SIZE * i));

			// The actual size of this block is either this calculated value or the size of a
			// ZSTDChunk, whichever is smaller.
			const std::size_t currChunkSize = std::min(bytesAvailableForChunk, ZSTD_INPUT_CHUNK_SIZE);
			
			const ZSTDChunk zChunk{
				.FileOffsetInBytes = assetTOCEntry.FileOffsetInBytes + (ZSTD_INPUT_CHUNK_SIZE * i),
				.ChunkSizeInBytes = currChunkSize
			};

			mRingBuffer.push_back(std::move(zChunk));
		}
	}

	const ZSTDChunk& ZSTDChunkRingBuffer::GetNextBlock()
	{
		assert(mCurrIndex < mRingBuffer.size() && "ERROR: An attempt was made to continue decompressing a BPK ZSTD archive after it was already entirely decompressed, but wrapping around was disabled!");

		const ZSTDChunk& nextBlock{ mRingBuffer[mCurrIndex] };

		if (mAllowWrapAround)
			mCurrIndex = (mCurrIndex + 1) % mRingBuffer.size();
		else
			++mCurrIndex;

		return nextBlock;
	}

	bool ZSTDChunkRingBuffer::IsFinished() const
	{
		return (!mAllowWrapAround && (mCurrIndex == mRingBuffer.size()));
	}
}

namespace Brawler
{
	BPKZSTDContext::BPKZSTDContext(const FilePathHash& pathHash, const DecompressionMode decompressionMode) :
		I_ZSTDContext(),
		mPathHash(pathHash),
		mMappedBPKView(),
		mZSTDChunkBuffer(pathHash, (decompressionMode == DecompressionMode::WRAP_AROUND))
	{}

	std::span<const std::uint8_t> BPKZSTDContext::FetchInputData(const std::size_t numBytesToFetch)
	{
		assert(!IsInputFinished() && "ERROR: BPKZSTDContext::FetchInputData() was called after all of the input has finished being decompressed!");

		// First, create a new mapping for the next input chunk which needs to be decompressed.
		ReMapFileView();

		return mMappedBPKView.GetMappedData();
	}

	bool BPKZSTDContext::IsInputFinished() const
	{
		return mZSTDChunkBuffer.IsFinished();
	}

	void BPKZSTDContext::ReMapFileView()
	{
		const IMPL::ZSTDChunk& currInputChunk{ mZSTDChunkBuffer.GetNextBlock() };
		const FileMapper& bpkMapper{ Util::BPK::GetBPKArchiveReader().GetBPKFileMapper() };

		mMappedBPKView = bpkMapper.CreateMappedFileView(currInputChunk.FileOffsetInBytes, currInputChunk.ChunkSizeInBytes);
	}
}