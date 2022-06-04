module;
#include <array>
#include <vector>
#include <span>
#include <atomic>

export module Brawler.AssetManagement.Win32AssetIORequestBuilder;
import Brawler.AssetManagement.I_AssetIORequestBuilder;
import Brawler.D3D12.I_BufferSubAllocation;
import Brawler.FilePathHash;
import Brawler.JobPriority;
import Brawler.AssetManagement.Win32AssetIORequest;
import Brawler.AssetManagement.AssetRequestEventHandle;
import Brawler.AssetManagement.AssetRequestEventNotifier;
import Brawler.AssetManagement.Win32AssetIORequestTracker;

export namespace Brawler
{
	namespace AssetManagement
	{
		class Win32AssetIORequestBuilder final : public I_AssetIORequestBuilder, private AssetRequestEventNotifier
		{
		private:
			using RequestContainer = std::vector<Win32AssetIORequest>;

		public:
			Win32AssetIORequestBuilder(AssetRequestEventHandle&& hAssetRequestEvent);

			Win32AssetIORequestBuilder(const Win32AssetIORequestBuilder& rhs) = delete;
			Win32AssetIORequestBuilder& operator=(const Win32AssetIORequestBuilder& rhs) = delete;

			Win32AssetIORequestBuilder(Win32AssetIORequestBuilder&& rhs) noexcept = default;
			Win32AssetIORequestBuilder& operator=(Win32AssetIORequestBuilder&& rhs) noexcept = default;

			void AddAssetIORequest(const Brawler::FilePathHash pathHash, Brawler::D3D12::I_BufferSubAllocation& bufferSubAllocation) override;

			void AddAssetIORequest(const CustomFileAssetIORequest& customFileRequest) override;

			std::span<Win32AssetIORequest> GetAssetIORequestSpan(const Brawler::JobPriority priority);
			std::span<const Win32AssetIORequest> GetAssetIORequestSpan(const Brawler::JobPriority priority) const;

			void Finalize();

			bool ReadyForDeletion() const;

		private:
			RequestContainer& GetCurrentRequestContainer();
			const RequestContainer& GetCurrentRequestContainer() const;

		private:
			std::array<std::vector<Win32AssetIORequest>, std::to_underlying(Brawler::JobPriority::COUNT)> mRequestContainerArr;
			Win32AssetIORequestTracker mRequestTracker;
		};
	}
}