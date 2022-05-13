module;
#include <DirectXTex.h>

export module Brawler.BC7CompressionEventHandle;

namespace Brawler
{
	class BC7ImageCompressor;
}

export namespace Brawler
{
	class BC7CompressionEventHandle
	{
	public:
		BC7CompressionEventHandle() = default;
		explicit BC7CompressionEventHandle(BC7ImageCompressor& compressor);

		~BC7CompressionEventHandle();

		BC7CompressionEventHandle(const BC7CompressionEventHandle& rhs) = delete;
		BC7CompressionEventHandle& operator=(const BC7CompressionEventHandle& rhs) = delete;

		BC7CompressionEventHandle(BC7CompressionEventHandle&& rhs) noexcept;
		BC7CompressionEventHandle& operator=(BC7CompressionEventHandle&& rhs) noexcept;

		bool TryCopyCompressedImage() const; 
		void SetDestinationImage(const DirectX::Image& destImage);

	private:
		void MarkCompressorForDeletion();

	private:
		BC7ImageCompressor* mCompressorPtr;
		const DirectX::Image* mDestImagePtr;
	};
}