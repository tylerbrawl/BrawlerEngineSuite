module;
#include <stdexcept>
#include <filesystem>
#include <format>
#include <array>
#include <memory>
#include <string_view>
#include <DxDef.h>

module Brawler.AssetManagement.DirectStorageAssetIORequestHandler;
import Brawler.AssetManagement.BPKArchiveReader;
import Util.General;
import Util.Engine;
import Brawler.AssetManagement.DirectStorageAssetIORequestBuilder;

namespace
{
	// This value might require some experimentation.
	static constexpr std::uint16_t DESIRED_DIRECT_STORAGE_QUEUE_CAPACITY = DSTORAGE_MAX_QUEUE_CAPACITY;
	static constexpr std::uint16_t ACTUAL_DIRECT_STORAGE_QUEUE_CAPACITY = std::clamp<std::uint16_t>(DESIRED_DIRECT_STORAGE_QUEUE_CAPACITY, DSTORAGE_MIN_QUEUE_CAPACITY, DSTORAGE_MAX_QUEUE_CAPACITY);

	consteval std::array<DSTORAGE_QUEUE_DESC, std::to_underlying(DSTORAGE_PRIORITY::DSTORAGE_PRIORITY_COUNT)> CreateDefaultDirectStorageQueueDescArray()
	{
		const DSTORAGE_QUEUE_DESC defaultQueueDesc{
			.SourceType = DSTORAGE_REQUEST_SOURCE_TYPE::DSTORAGE_REQUEST_SOURCE_FILE,
			.Capacity = ACTUAL_DIRECT_STORAGE_QUEUE_CAPACITY,
			.Priority{},
			.Name{ nullptr },
			.Device{ nullptr }
		};
		std::array<DSTORAGE_QUEUE_DESC, std::to_underlying(DSTORAGE_PRIORITY::DSTORAGE_PRIORITY_COUNT)> queueDescArr{
			defaultQueueDesc,
			defaultQueueDesc,
			defaultQueueDesc,
			defaultQueueDesc
		};

		DSTORAGE_PRIORITY currPriority = DSTORAGE_PRIORITY::DSTORAGE_PRIORITY_FIRST;
		for (auto& queueDesc : queueDescArr)
		{
			queueDesc.Priority = currPriority;
			currPriority = static_cast<DSTORAGE_PRIORITY>(std::to_underlying(currPriority) + 1);
		}

		if constexpr (Util::General::GetBuildMode() != Util::General::BuildMode::RELEASE)
		{
			queueDescArr[static_cast<std::size_t>(DSTORAGE_PRIORITY::DSTORAGE_PRIORITY_LOW) + 1] = "DirectStorage Queue - Low Priority";
			queueDescArr[static_cast<std::size_t>(DSTORAGE_PRIORITY::DSTORAGE_PRIORITY_NORMAL) + 1] = "DirectStorage Queue - Normal Priority";
			queueDescArr[static_cast<std::size_t>(DSTORAGE_PRIORITY::DSTORAGE_PRIORITY_HIGH) + 1] = "DirectStorage Queue - High Priority";
			queueDescArr[static_cast<std::size_t>(DSTORAGE_PRIORITY::DSTORAGE_PRIORITY_REALTIME) + 1] = "DirectStorage Queue - Real-Time Priority";
		}

		return defaultQueueDesc;
	}
}

namespace Brawler
{
	namespace AssetManagement
	{
		DirectStorageAssetIORequestHandler::DirectStorageAssetIORequestHandler(Microsoft::WRL::ComPtr<IDStorageFactory>&& directStorageFactory) :
			I_AssetIORequestHandler(),
			mDStorageFactory(std::move(directStorageFactory)),
			mBPKDStorageFile(nullptr),
			mDecompressionEventQueue(nullptr),
			mDStorageQueueArr(),
			mDStorageStatusArray(nullptr)
		{
			CreateBPKArchiveDirectStorageFile();
			CreateDecompressionEventQueue();
			CreateDirectStorageQueues();
			CreateStatusArray();
		}

		void DirectStorageAssetIORequestHandler::PrepareAssetIORequests(std::unique_ptr<EnqueuedAssetDependency>&& enqueuedDependency)
		{
			// Let the asset dependency resolver callbacks in the AssetDependency tell us which assets need to
			// be loaded.
			DirectStorageAssetIORequestBuilder requestBuilder{ *(mBPKDStorageFile.Get()) };
			enqueuedDependency->Dependency.BuildAssetIORequests(requestBuilder);


		}

		void DirectStorageAssetIORequestHandler::CreateBPKArchiveDirectStorageFile()
		{
			const std::filesystem::path& bpkArchivePath{ BPKArchiveReader::GetBPKArchivePath() };

			std::error_code errorCode{};
			const bool bpkArchiveExists = std::filesystem::exists(bpkArchivePath, errorCode);

			if (errorCode) [[unlikely]]
				throw std::runtime_error{ std::format("ERROR: The attempt to check if the BPK archive exists for DirectStorage usage failed with the following error: {}", errorCode.message()) };

			if (!bpkArchiveExists) [[unlikely]]
				throw std::runtime_error{ "ERROR: The BPK data archive could not be found!" };

			Util::General::CheckHRESULT(mDStorageFactory->OpenFile(bpkArchivePath.c_str(), IID_PPV_ARGS(&mBPKDStorageFile)));
		}

		void DirectStorageAssetIORequestHandler::CreateDecompressionEventQueue()
		{
			// For some reason, we have to get the decompression queue by using QueryInterface()
			// on the IDStorageFactory. Don't ask me why.
			Util::General::CheckHRESULT(mDStorageFactory.As(&mDecompressionEventQueue));
		}

		void DirectStorageAssetIORequestHandler::CreateDirectStorageQueues()
		{
			using QueueDescArr_T = std::array<DSTORAGE_QUEUE_DESC, std::to_underlying(DSTORAGE_PRIORITY::DSTORAGE_PRIORITY_COUNT)>;
			
			static constexpr QueueDescArr_T DEFAULT_QUEUE_DESC_ARR{ CreateDefaultDirectStorageQueueDescArray() };
			QueueDescArr_T modifiableQueueDescArr{ DEFAULT_QUEUE_DESC_ARR };

			{
				Brawler::D3D12Device* const d3dDevicePtr = &(Util::Engine::GetD3D12Device());

				for (auto& queueDesc : modifiableQueueDescArr)
					queueDesc.Device = d3dDevicePtr;
			}
			
			for (std::size_t i = 0; i < static_cast<std::size_t>(DSTORAGE_PRIORITY::DSTORAGE_PRIORITY_COUNT); ++i)
			{
				Microsoft::WRL::ComPtr<IDStorageQueue>& currQueue{ mDStorageQueueArr[i] };
				Util::General::CheckHRESULT(mDStorageFactory->CreateQueue(&(modifiableQueueDescArr[i]), IID_PPV_ARGS(&currQueue)));
			}
		}

		void DirectStorageAssetIORequestHandler::CreateStatusArray()
		{
			static constexpr std::uint32_t MAX_STATUS_ENTRIES = (static_cast<std::uint32_t>(ACTUAL_DIRECT_STORAGE_QUEUE_CAPACITY) * static_cast<std::uint32_t>(DSTORAGE_PRIORITY::DSTORAGE_PRIORITY_COUNT));

			if constexpr (Util::General::GetBuildMode() != Util::General::BuildMode::RELEASE)
				Util::General::CheckHRESULT(mDStorageFactory->CreateStatusArray(MAX_STATUS_ENTRIES, "DirectStorage Status Array", IID_PPV_ARGS(&mDStorageStatusArray)));
			else
				Util::General::CheckHRESULT(mDStorageFactory->CreateStatusArray(MAX_STATUS_ENTRIES, nullptr, IID_PPV_ARGS(&mDStorageStatusArray)));
		}
	}
}