module;
#include <span>
#include <functional>
#include <cassert>

module Brawler.AssetManagement.Win32AssetIORequest;
import Brawler.AssetManagement.BPKArchiveReader;
import Brawler.MappedFileView;
import Brawler.FileAccessMode;

namespace Brawler
{
	namespace AssetManagement
	{
		Win32AssetIORequest::Win32AssetIORequest(Brawler::FilePathHash pathHash, Win32AssetIORequestTracker& requestTracker) :
			mWriteDataCallback(),
			mPathHash(std::move(pathHash)),
			mRequestTrackerPtr(std::addressof(requestTracker))
		{}

		void Win32AssetIORequest::SetWriteDataCallback(WriteDataCallback_T&& callback)
		{
			mWriteDataCallback = std::move(callback);
		}

		void Win32AssetIORequest::LoadAssetData()
		{
			// Create the mapped file view for the source data.
			MappedFileView<FileAccessMode::READ_ONLY> sourceDataView{ BPKArchiveReader::GetInstance().CreateMappedFileViewForAsset(mPathHash) };
			const std::span<const std::byte> srcDataSpan{ sourceDataView.GetMappedData() };

			mWriteDataCallback(srcDataSpan);

			assert(mRequestTrackerPtr != nullptr && "ERROR: A Win32AssetIORequest instance was never given an associated Win32AssetIORequestTracker& before Win32AssetIORequest::LoadAssetData() was called!");
			mRequestTrackerPtr->NotifyForAssetIORequestCompletion();
		}
	}
}