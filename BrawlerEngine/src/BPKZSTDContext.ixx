module;
#include <vector>
#include <span>

export module Brawler.BPKZSTDContext;
import Brawler.MappedFileView;
import Brawler.I_ZSTDContext;
import Brawler.FilePathHash;

namespace IMPL
{
	struct ZSTDChunk
	{
		std::uint64_t FileOffsetInBytes;
		std::uint64_t ChunkSizeInBytes;
	};

	class ZSTDChunkRingBuffer
	{
	public:
		ZSTDChunkRingBuffer(const Brawler::FilePathHash& pathHash, bool allowWrapAround);

	private:
		void InitializeRingBuffer(const Brawler::FilePathHash& pathHash);

	public:
		const ZSTDChunk& GetNextBlock();
		bool IsFinished() const;

	private:
		std::vector<ZSTDChunk> mRingBuffer;
		std::size_t mCurrIndex;
		bool mAllowWrapAround;
	};
}

export namespace Brawler
{
	class BPKZSTDContext : public I_ZSTDContext
	{
	public:
		enum class DecompressionMode
		{
			/// <summary>
			/// When the entire archive has been decompressed, no more decompression
			/// will take place.
			/// </summary>
			DECOMPRESS_ONCE,

			/// <summary>
			/// When the entire archive has been decompressed, decompression will start
			/// again immediately at the beginning of the archive.
			/// </summary>
			WRAP_AROUND
		};

	public:
		BPKZSTDContext(const FilePathHash& pathHash, const DecompressionMode decompressionMode = DecompressionMode::DECOMPRESS_ONCE);

	protected:
		std::span<const std::uint8_t> FetchInputData(const std::size_t numBytesToFetch) override;
		bool IsInputFinished() const override;

	private:
		void ReMapFileView();

	private:
		FilePathHash mPathHash;
		MappedFileView mMappedBPKView;
		IMPL::ZSTDChunkRingBuffer mZSTDChunkBuffer;
	};
}