module;
#include <span>
#include <functional>

export module Brawler.AssetManagement.Win32AssetIORequest;
import Brawler.CompositeEnum;
import Brawler.FilePathHash;
import Brawler.AssetManagement.Win32AssetIORequestTracker;

export namespace Brawler
{
	namespace AssetManagement
	{
		class Win32AssetIORequest
		{
		private:
			using WriteDataCallback_T = std::move_only_function<void(const std::span<const std::byte>)>;

		public:
			Win32AssetIORequest() = default;
			Win32AssetIORequest(Brawler::FilePathHash pathHash, Win32AssetIORequestTracker& requestTracker);

			Win32AssetIORequest(const Win32AssetIORequest& rhs) = delete;
			Win32AssetIORequest& operator=(const Win32AssetIORequest& rhs) = delete;

			Win32AssetIORequest(Win32AssetIORequest&& rhs) noexcept = default;
			Win32AssetIORequest& operator=(Win32AssetIORequest&& rhs) noexcept = default;

			void SetWriteDataCallback(WriteDataCallback_T&& callback);

			void LoadAssetData();

		private:
			WriteDataCallback_T mWriteDataCallback;
			Brawler::FilePathHash mPathHash;
			Win32AssetIORequestTracker* mRequestTrackerPtr;
		};
	}
}