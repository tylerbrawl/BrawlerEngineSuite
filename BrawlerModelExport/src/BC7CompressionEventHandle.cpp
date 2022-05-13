module;
#include <cassert>
#include <DirectXTex.h>

module Brawler.BC7CompressionEventHandle;
import Brawler.BC7ImageCompressor;

namespace Brawler
{
	BC7CompressionEventHandle::BC7CompressionEventHandle(BC7ImageCompressor& compressor) :
		mCompressorPtr(&compressor),
		mDestImagePtr(nullptr)
	{}

	BC7CompressionEventHandle::~BC7CompressionEventHandle()
	{
		MarkCompressorForDeletion();
	}

	BC7CompressionEventHandle::BC7CompressionEventHandle(BC7CompressionEventHandle&& rhs) noexcept :
		mCompressorPtr(rhs.mCompressorPtr),
		mDestImagePtr(rhs.mDestImagePtr)
	{
		rhs.mCompressorPtr = nullptr;
		rhs.mDestImagePtr = nullptr;
	}

	BC7CompressionEventHandle& BC7CompressionEventHandle::operator=(BC7CompressionEventHandle&& rhs) noexcept
	{
		MarkCompressorForDeletion();

		mCompressorPtr = rhs.mCompressorPtr;
		rhs.mCompressorPtr = nullptr;

		mDestImagePtr = rhs.mDestImagePtr;
		rhs.mDestImagePtr = nullptr;

		return *this;
	}

	bool BC7CompressionEventHandle::TryCopyCompressedImage() const
	{
		assert(mCompressorPtr != nullptr && "ERROR: A BC7CompressionEventHandle instance was never assigned a corresponding BC7ImageCompressor*!");
		assert(mDestImagePtr != nullptr && "ERROR: BC7CompressionEventHandle::TryCopyCompressedImage() was called before the BC7CompressionEventHandle was assigned a destination DirectX::Image!");

		return mCompressorPtr->TryCopyCompressedImage(*mDestImagePtr);
	}

	void BC7CompressionEventHandle::SetDestinationImage(const DirectX::Image& destImage)
	{
		mDestImagePtr = &destImage;
	}

	void BC7CompressionEventHandle::MarkCompressorForDeletion()
	{
		if (mCompressorPtr != nullptr) [[likely]]
		{
			mCompressorPtr->MarkForDeletion();
			mCompressorPtr = nullptr;
		}
	}
}