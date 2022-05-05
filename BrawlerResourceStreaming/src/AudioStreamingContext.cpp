module;
#include <array>
#include <span>
#include <optional>
#include <memory>
#include <vector>
#include <stdexcept>
#include <ranges>
#include "Win32Def.h"

module Brawler.AudioStreamingContext;
import Brawler.StreamedAudio;
import Brawler.StreamedAudioVoice;

namespace
{
	static constexpr std::string_view RIFF_MAGIC = "RIFF";
	static constexpr std::string_view FMT_MAGIC = "fmt ";
	static constexpr std::string_view DATA_MAGIC = "data";

	static constexpr std::size_t RIFF_CHUNK_SIZE = 12;

	class RIFFWAVEFormatHandlerState final : public IMPL::I_WAVEFormatHandlerState
	{
	public:
		explicit RIFFWAVEFormatHandlerState(IMPL::WAVEFormatHandler& owningHandler);

		std::span<const std::uint8_t> GetAudioData(std::span<const std::uint8_t>& dataArr) override;
		bool IsFinished() const override;

	private:
		bool IsFileValid() const;

	private:
		std::array<std::uint8_t, RIFF_CHUNK_SIZE> mByteArr;
		std::size_t mBytesCopied;
	};

	static constexpr std::size_t FMT_HEADER_SIZE = 8;

	class FMTWAVEFormatHandlerState final : public IMPL::I_WAVEFormatHandlerState
	{
	public:
		explicit FMTWAVEFormatHandlerState(IMPL::WAVEFormatHandler& owningHandler);

		std::span<const std::uint8_t> GetAudioData(std::span<const std::uint8_t>& dataArr) override;
		bool IsFinished() const override;

	private:
		void TryInitializeFMTChunkInfo(std::span<const std::uint8_t>& dataArr);
		void TryInitializeWAVEFormatStructure(std::span<const std::uint8_t> dataArr);

	private:
		std::vector<std::uint8_t> mByteArr;
		std::uint32_t mChunkSize;
		std::size_t mBytesCopied;
	};

	static constexpr std::size_t DATA_HEADER_SIZE = 8;

	class AudioDataWAVEFormatHandlerState final : public IMPL::I_WAVEFormatHandlerState
	{
	public:
		explicit AudioDataWAVEFormatHandlerState(IMPL::WAVEFormatHandler& owningHandler);

		std::span<const std::uint8_t> GetAudioData(std::span<const std::uint8_t>& dataArr) override;
		bool IsFinished() const override;

	private:
		void TryInitializeDataChunkInfo(std::span<const std::uint8_t>& dataArr);

	private:
		std::array<std::uint8_t, DATA_HEADER_SIZE> mByteArr;
		std::uint32_t mChunkSize;
		std::size_t mHeaderBytesCopied;
		std::uint32_t mDataBytesRead;
	};

	// ------------------------------------------------------------------------------------------------------
	// ^ WAVEFormatHandlerState Declarations           /           WAVEFormatHandlerState Definitions v
	// ------------------------------------------------------------------------------------------------------

	RIFFWAVEFormatHandlerState::RIFFWAVEFormatHandlerState(IMPL::WAVEFormatHandler& owningHandler) :
		IMPL::I_WAVEFormatHandlerState(owningHandler),
		mByteArr(),
		mBytesCopied(0)
	{}

	std::span<const std::uint8_t> RIFFWAVEFormatHandlerState::GetAudioData(std::span<const std::uint8_t>& dataArr)
	{
		const std::size_t bytesToCopy = std::min((RIFF_CHUNK_SIZE - mBytesCopied), dataArr.size_bytes());
		std::memcpy(reinterpret_cast<void*>(mByteArr.data() + mBytesCopied), reinterpret_cast<const void*>(dataArr.data()), bytesToCopy);

		mBytesCopied += bytesToCopy;

		if (mBytesCopied == mByteArr.size() && !IsFileValid())
			throw std::runtime_error{ "ERROR: An invalid RIFF (WAVE) file was provided for streaming!" };

		dataArr = dataArr | std::views::drop(mBytesCopied);
		
		return std::span<const std::uint8_t>{};
	}

	bool RIFFWAVEFormatHandlerState::IsFinished() const
	{
		return (mBytesCopied == mByteArr.size());
	}

	bool RIFFWAVEFormatHandlerState::IsFileValid() const
	{
		// The first four characters should be "RIFF."

		// The next four bytes are the size of the file minus the first eight bytes.

		// After that, the next four characters should be "WAVE." We do not currently
		// support WXMA files.
		static constexpr std::string_view WAVE_MAGIC = "WAVE";

		// This would be a perfect use for std::views::zip...
		for (std::size_t i = 0; i < RIFF_MAGIC.size(); ++i)
		{
			if (mByteArr[i] != RIFF_MAGIC[i]) [[unlikely]]
				return false;
		}

		std::span<const std::uint8_t> fileTypeSpan{ mByteArr | std::views::drop(8) };

		// Again, std::views::zip would be excellent here.
		for (std::size_t i = 0; i < WAVE_MAGIC.size(); ++i)
		{
			if (fileTypeSpan[i] != WAVE_MAGIC[i]) [[unlikely]]
				return false;
		}

		return true;
	}

	FMTWAVEFormatHandlerState::FMTWAVEFormatHandlerState(IMPL::WAVEFormatHandler& owningHandler) :
		IMPL::I_WAVEFormatHandlerState(owningHandler),
		mByteArr(),
		mChunkSize(0),
		mBytesCopied(0)
	{
		mByteArr.resize(FMT_HEADER_SIZE);
	}

	std::span<const std::uint8_t> FMTWAVEFormatHandlerState::GetAudioData(std::span<const std::uint8_t>& dataArr)
	{
		if (mChunkSize == 0)
			TryInitializeFMTChunkInfo(dataArr);

		// We don't use else here because TryInitializeFMTChunkInfo may have changed
		// mChunkSize.
		if (mChunkSize != 0)
			TryInitializeWAVEFormatStructure(dataArr);

		dataArr = dataArr | std::views::drop(mChunkSize);

		return std::span<const std::uint8_t>{};
	}

	bool FMTWAVEFormatHandlerState::IsFinished() const
	{
		return ((mChunkSize != 0) && (mBytesCopied == mChunkSize));
	}

	void FMTWAVEFormatHandlerState::TryInitializeFMTChunkInfo(std::span<const std::uint8_t>& dataArr)
	{
		const std::size_t bytesToCopy = std::min((FMT_HEADER_SIZE - mBytesCopied), dataArr.size_bytes());
		std::memcpy(reinterpret_cast<void*>(mByteArr.data() + mBytesCopied), reinterpret_cast<const void*>(dataArr.data()), bytesToCopy);

		mBytesCopied += bytesToCopy;

		if (mBytesCopied == FMT_HEADER_SIZE)
		{
			// We have enough information to validate the FMT chunk. The first four bytes should be
			// "fmt " (including the space character as a byte).
			std::span<const std::uint8_t> magicSpan{ mByteArr | std::views::take(4) };

			for (std::size_t i = 0; i < FMT_MAGIC.size(); ++i)
			{
				if (magicSpan[i] != FMT_MAGIC[i]) [[unlikely]]
					throw std::runtime_error{ "ERROR: An invalid RIFF (WAVE) file was provided for streaming!" };
			}

			// The next four bytes is the size of the FMT data.
			std::span<const std::uint8_t> sizeSpan{ mByteArr | std::views::drop(4) | std::views::take(4) };
			std::memcpy(reinterpret_cast<void*>(&mChunkSize), reinterpret_cast<const void*>(sizeSpan.data()), sizeof(mChunkSize));

			// Re-use the mByteArr vector to store the data for the WAVEFORMATEXTENSIBLE structure.
			mByteArr.clear();
			mBytesCopied = 0;

			dataArr = dataArr.subspan(8);
		}
	}

	void FMTWAVEFormatHandlerState::TryInitializeWAVEFormatStructure(std::span<const std::uint8_t> dataArr)
	{
		const std::size_t bytesToCopy = std::min((static_cast<std::size_t>(mChunkSize) - mBytesCopied), dataArr.size_bytes());
		std::memcpy(reinterpret_cast<void*>(mByteArr.data() + mBytesCopied), reinterpret_cast<const void*>(dataArr.data()), bytesToCopy);

		mBytesCopied += bytesToCopy;

		if (mBytesCopied == mChunkSize)
		{
			WAVEFORMATEXTENSIBLE waveFormatExt{};
			std::memcpy(reinterpret_cast<void*>(&waveFormatExt), reinterpret_cast<const void*>(mByteArr.data()), mChunkSize);

			std::optional<WAVEFORMATEXTENSIBLE>& handlerWaveFormat{ GetWAVEFormatHandler().GetWaveFormat() };
			handlerWaveFormat = waveFormatExt;
		}
	}

	AudioDataWAVEFormatHandlerState::AudioDataWAVEFormatHandlerState(IMPL::WAVEFormatHandler& owningHandler) :
		I_WAVEFormatHandlerState(owningHandler),
		mByteArr(),
		mChunkSize(0),
		mHeaderBytesCopied(0),
		mDataBytesRead(0)
	{}

	std::span<const std::uint8_t> AudioDataWAVEFormatHandlerState::GetAudioData(std::span<const std::uint8_t>& dataArr)
	{
		if (mHeaderBytesCopied < DATA_HEADER_SIZE)
			TryInitializeDataChunkInfo(dataArr);

		// If called, TryInitializeDataChunkInfo() will advance the dataArr span to the beginning
		// of the audio data. So, we can always assume that the span points to valid audio data.

		mDataBytesRead += static_cast<std::uint32_t>(dataArr.size());

		std::span<const std::uint8_t> audioDataSpan{ dataArr };
		dataArr = std::span<const std::uint8_t>{};

		return audioDataSpan;
	}

	bool AudioDataWAVEFormatHandlerState::IsFinished() const
	{
		return (mDataBytesRead == mChunkSize);
	}

	void AudioDataWAVEFormatHandlerState::TryInitializeDataChunkInfo(std::span<const std::uint8_t>& dataArr)
	{
		const std::size_t bytesToCopy = std::min((DATA_HEADER_SIZE - mHeaderBytesCopied), dataArr.size_bytes());
		std::memcpy(reinterpret_cast<void*>(mByteArr.data() + mHeaderBytesCopied), reinterpret_cast<const void*>(dataArr.data()), bytesToCopy);

		mHeaderBytesCopied += bytesToCopy;

		if (mHeaderBytesCopied == DATA_HEADER_SIZE)
		{
			// The first four bytes should be "data."

			std::span<const std::uint8_t> magicSpan{ mByteArr | std::views::take(4) };

			for (std::size_t i = 0; i < DATA_MAGIC.size(); ++i)
			{
				if (magicSpan[i] != DATA_MAGIC[i]) [[unlikely]]
					throw std::runtime_error{ "ERROR: An invalid RIFF (WAVE) file was provided for streaming!" };
			}

			// The next four bytes represents the size, in bytes, of the audio data.
			std::span<const std::uint8_t> dataSizeSpan{ mByteArr | std::views::drop(4) };
			std::memcpy(reinterpret_cast<void*>(&mChunkSize), reinterpret_cast<const void*>(dataSizeSpan.data()), sizeof(mChunkSize));
		}

		dataArr = dataArr.subspan(bytesToCopy);
	}
}

namespace IMPL
{
	WAVEDataRingBuffer::WAVEDataRingBuffer() :
		mDecodedZSTDBlockArr(),
		mCurrLoadIndex(0),
		mCurrRetrievalIndex(0),
		mLoadedDataCount(0)
	{}

	void WAVEDataRingBuffer::StoreLoadedDataBlock(std::vector<std::uint8_t>&& data)
	{
		mDecodedZSTDBlockArr[mCurrLoadIndex] = std::move(data);
		mCurrLoadIndex = ((mCurrLoadIndex + 1) % mDecodedZSTDBlockArr.size());

		mLoadedDataCount = std::min(mLoadedDataCount + 1, mDecodedZSTDBlockArr.size());
	}

	std::span<const std::uint8_t> WAVEDataRingBuffer::GetNextLoadedDataBlock(bool advanceRetrievalIndex)
	{
		if (mLoadedDataCount == 0 || (mCurrLoadIndex == mCurrRetrievalIndex))
			return std::span<const std::uint8_t>{};

		std::span<const std::uint8_t> dataBlock{ mDecodedZSTDBlockArr[mCurrRetrievalIndex] };

		if (advanceRetrievalIndex)
			mCurrRetrievalIndex = ((mCurrRetrievalIndex + 1) % mDecodedZSTDBlockArr.size());

		return dataBlock;
	}

	I_WAVEFormatHandlerState::I_WAVEFormatHandlerState(WAVEFormatHandler& owningHandler) :
		mOwningHandler(&owningHandler)
	{}

	WAVEFormatHandler& I_WAVEFormatHandlerState::GetWAVEFormatHandler()
	{
		return *mOwningHandler;
	}

	const WAVEFormatHandler& I_WAVEFormatHandlerState::GetWAVEFormatHandler() const
	{
		return *mOwningHandler;
	}

	WAVEFormatHandler::WAVEFormatHandler() :
		mWaveFormatEx(),
		mState(nullptr)
	{}

	std::span<const std::uint8_t> WAVEFormatHandler::GetAudioData(std::span<const std::uint8_t> dataArr)
	{
		std::span<const std::uint8_t> audioDataSpan{};

		while (!dataArr.empty())
		{
			if (mState == nullptr || mState->IsFinished())
				InitializeState(dataArr);

			audioDataSpan = mState->GetAudioData(dataArr);
		}
		
		return audioDataSpan;
	}

	std::optional<WAVEFORMATEXTENSIBLE>& WAVEFormatHandler::GetWaveFormat()
	{
		return mWaveFormatEx;
	}

	const std::optional<WAVEFORMATEXTENSIBLE>& WAVEFormatHandler::GetWaveFormat() const
	{
		return mWaveFormatEx;
	}

	void WAVEFormatHandler::ResetState()
	{
		mState = nullptr;
	}

	void WAVEFormatHandler::InitializeState(std::span<const std::uint8_t> dataArr)
	{
		// We have an error if there are not at least four characters present.
		if (dataArr.size() < 4) [[unlikely]]
			throw std::runtime_error{ "ERROR: An invalid RIFF (WAVE) file was provided for streaming!" };

		std::span<const std::uint8_t> magicSpan{ dataArr | std::views::take(4) };

		{
			bool isRIFFChunk = true;

			for (std::size_t i = 0; i < RIFF_MAGIC.size(); ++i)
			{
				if (magicSpan[i] != RIFF_MAGIC[i])
				{
					isRIFFChunk = false;
					break;
				}
			}

			if (isRIFFChunk)
			{
				mState = std::make_unique<RIFFWAVEFormatHandlerState>(*this);
				return;
			}
		}

		{
			bool isFMTChunk = true;

			for (std::size_t i = 0; i < FMT_MAGIC.size(); ++i)
			{
				if (magicSpan[i] != FMT_MAGIC[i])
				{
					isFMTChunk = false;
					break;
				}
			}

			if (isFMTChunk)
			{
				mState = std::make_unique<FMTWAVEFormatHandlerState>(*this);
				return;
			}
		}

		// If it is neither an RIFF nor an FMT chunk, then we assume that it is audio
		// data.
		mState = std::make_unique<AudioDataWAVEFormatHandlerState>(*this);
	}
}

namespace Brawler
{
	AudioStreamingContext::AudioStreamingContext(StreamedAudio& audioAsset) :
		mZSTDContext(audioAsset.GetFilePathHash(), BPKZSTDContext::DecompressionMode::WRAP_AROUND),
		mDataRingBuffer(),
		mDataHandler(),
		mAudioAsset(&audioAsset)
	{}

	void AudioStreamingContext::RequestAudioDataLoad(const std::size_t numBlocks)
	{
		const std::size_t numBytesToLoad = (numBlocks * mZSTDContext.GetBlockSize());
		mAudioAsset->RequestAudioDataLoad(*this, numBytesToLoad);
	}

	const std::optional<WAVEFORMATEXTENSIBLE>& AudioStreamingContext::GetWaveFormat()
	{
		if (!mDataHandler.GetWaveFormat().has_value())
		{
			std::span<const std::uint8_t> decompressedHeaderBlock{ mDataRingBuffer.GetNextLoadedDataBlock(false) };

			if (!decompressedHeaderBlock.empty())
			{
				mDataHandler.GetAudioData(decompressedHeaderBlock);
				mDataHandler.ResetState();
			}
		}
		
		return mDataHandler.GetWaveFormat();
	}

	std::optional<XAUDIO2_BUFFER> AudioStreamingContext::GetXAudio2Buffer()
	{
		assert(GetWaveFormat().has_value() && "ERROR: An attempt was made to get an XAUDIO2_BUFFER before the file format could be verified!");

		std::span<const std::uint8_t> audioBufferSpan{ mDataRingBuffer.GetNextLoadedDataBlock() };
		if (audioBufferSpan.empty())
			return std::optional<XAUDIO2_BUFFER>{};

		audioBufferSpan = mDataHandler.GetAudioData(audioBufferSpan);
		if (audioBufferSpan.empty())
			return std::optional<XAUDIO2_BUFFER>{};

		XAUDIO2_BUFFER xAudioBuffer{
			.AudioBytes = static_cast<std::uint32_t>(audioBufferSpan.size_bytes()),
			.pAudioData = audioBufferSpan.data()
		};

		return std::optional<XAUDIO2_BUFFER>{ std::move(xAudioBuffer) };
	}

	void AudioStreamingContext::LoadAudioData(const std::size_t numBytesToLoad)
	{
		std::size_t blocksToLoad = (numBytesToLoad / mZSTDContext.GetBlockSize());

		while (blocksToLoad > 0)
		{
			std::vector<std::uint8_t> decompressedBlock{ mZSTDContext.DecompressData() };
			mDataRingBuffer.StoreLoadedDataBlock(std::move(decompressedBlock));

			--blocksToLoad;
		}
	}
}