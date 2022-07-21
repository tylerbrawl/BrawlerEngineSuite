module;
#include <span>
#include <functional>
#include <cassert>
#include <filesystem>

module Brawler.AssetManagement.Win32AssetIORequest;
import Brawler.AssetManagement.BPKArchiveReader;

namespace Brawler
{
	namespace AssetManagement
	{
		Win32AssetIORequest::Win32AssetIORequest(Brawler::FilePathHash pathHash, Win32AssetIORequestTracker& requestTracker) :
			mWriteDataCallback(),
			mMappedFileView(BPKArchiveReader::GetInstance().CreateMappedFileViewForAsset(pathHash)),
			mRequestTrackerPtr(&requestTracker)
		{}

		Win32AssetIORequest::Win32AssetIORequest(const CustomFileAssetIORequest& customFileRequest, Win32AssetIORequestTracker& requestTracker) :
			mWriteDataCallback(),
			mMappedFileView(customFileRequest.FilePath, MappedFileView<FileAccessMode::READ_ONLY>::ViewParams{
				.FileOffsetInBytes = customFileRequest.FileOffset,
				.ViewSizeInBytes = (customFileRequest.CompressedDataSizeInBytes == 0 ? customFileRequest.UncompressedDataSizeInBytes : customFileRequest.CompressedDataSizeInBytes)
			}),
			mRequestTrackerPtr(&requestTracker)
		{}

		Win32AssetIORequest::Win32AssetIORequest(Win32AssetIORequest&& rhs) noexcept :
			mWriteDataCallback(std::move(rhs.mWriteDataCallback)),
			mMappedFileView(std::move(rhs.mMappedFileView)),
			mRequestTrackerPtr(rhs.mRequestTrackerPtr)
		{
			rhs.mRequestTrackerPtr = nullptr;
		}

		Win32AssetIORequest& Win32AssetIORequest::operator=(Win32AssetIORequest&& rhs) noexcept
		{
			mWriteDataCallback = std::move(rhs.mWriteDataCallback);

			mMappedFileView = std::move(rhs.mMappedFileView);

			mRequestTrackerPtr = rhs.mRequestTrackerPtr;
			rhs.mRequestTrackerPtr = nullptr;

			return *this;
		}

		void Win32AssetIORequest::SetWriteDataCallback(WriteDataCallback_T&& callback)
		{
			mWriteDataCallback = std::move(callback);
		}

		void Win32AssetIORequest::LoadAssetData()
		{
			// Create the mapped file view for the source data.
			const std::span<const std::byte> srcDataSpan{ mMappedFileView.GetMappedData() };

			mWriteDataCallback(srcDataSpan);

			assert(mRequestTrackerPtr != nullptr && "ERROR: A Win32AssetIORequest instance was never given an associated Win32AssetIORequestTracker& before Win32AssetIORequest::LoadAssetData() was called!");
			mRequestTrackerPtr->NotifyForAssetIORequestCompletion();
		}
	}
}