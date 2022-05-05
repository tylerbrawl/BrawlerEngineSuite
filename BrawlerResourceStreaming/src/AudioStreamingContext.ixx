module;
#include <array>
#include <span>
#include <optional>
#include <memory>
#include <vector>
#include "Win32Def.h"

export module Brawler.AudioStreamingContext;
import Brawler.BPKZSTDContext;
import Brawler.FilePathHash;

export namespace Brawler
{
	class StreamedAudio;
	class StreamedAudioVoice;
}

namespace IMPL
{
	static constexpr std::size_t ZSTD_BLOCK_ARRAY_SIZE = 3;

	class WAVEDataRingBuffer
	{
	public:
		WAVEDataRingBuffer();

		void StoreLoadedDataBlock(std::vector<std::uint8_t>&& data);
		std::span<const std::uint8_t> GetNextLoadedDataBlock(bool advanceRetrievalIndex = true);

	private:
		std::array<std::vector<std::uint8_t>, ZSTD_BLOCK_ARRAY_SIZE> mDecodedZSTDBlockArr;
		std::size_t mCurrLoadIndex;
		std::size_t mCurrRetrievalIndex;
		std::size_t mLoadedDataCount;
	};

	class WAVEFormatHandler;

	class I_WAVEFormatHandlerState
	{
	protected:
		explicit I_WAVEFormatHandlerState(WAVEFormatHandler& owningHandler);

	public:
		virtual std::span<const std::uint8_t> GetAudioData(std::span<const std::uint8_t>& dataArr) = 0;
		virtual bool IsFinished() const = 0;

	protected:
		WAVEFormatHandler& GetWAVEFormatHandler();
		const WAVEFormatHandler& GetWAVEFormatHandler() const;

	private:
		WAVEFormatHandler* mOwningHandler;
	};

	class WAVEFormatHandler
	{
	public:
		WAVEFormatHandler();

		WAVEFormatHandler(const WAVEFormatHandler& rhs) = delete;
		WAVEFormatHandler& operator=(const WAVEFormatHandler& rhs) = delete;

		WAVEFormatHandler(WAVEFormatHandler&& rhs) noexcept = default;
		WAVEFormatHandler& operator=(WAVEFormatHandler&& rhs) noexcept = default;

		std::span<const std::uint8_t> GetAudioData(std::span<const std::uint8_t> dataArr);

		std::optional<WAVEFORMATEXTENSIBLE>& GetWaveFormat();
		const std::optional<WAVEFORMATEXTENSIBLE>& GetWaveFormat() const;

		void ResetState();

	private:
		void InitializeState(std::span<const std::uint8_t> dataArr);

	private:
		std::optional<WAVEFORMATEXTENSIBLE> mWaveFormatEx;
		std::unique_ptr<I_WAVEFormatHandlerState> mState;
	};
}

export namespace Brawler
{
	class AudioStreamingContext
	{
	private:
		friend class StreamedAudio;

	public:
		explicit AudioStreamingContext(StreamedAudio& audioAsset);

		AudioStreamingContext(const AudioStreamingContext& rhs) = delete;
		AudioStreamingContext& operator=(const AudioStreamingContext& rhs) = delete;

		AudioStreamingContext(AudioStreamingContext&& rhs) noexcept = default;
		AudioStreamingContext& operator=(AudioStreamingContext&& rhs) noexcept = default;

		void RequestAudioDataLoad(const std::size_t numBlocks = 1);

		const std::optional<WAVEFORMATEXTENSIBLE>& GetWaveFormat();

		std::optional<XAUDIO2_BUFFER> GetXAudio2Buffer();

	private:
		void LoadAudioData(const std::size_t numBytesToLoad);

	private:
		BPKZSTDContext mZSTDContext;
		IMPL::WAVEDataRingBuffer mDataRingBuffer;
		IMPL::WAVEFormatHandler mDataHandler;
		StreamedAudio* mAudioAsset;
	};
}