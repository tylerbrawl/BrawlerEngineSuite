module;
#include <memory>
#include <vector>
#include <ranges>
#include <cassert>
#include <format>
#include <filesystem>
#include <fstream>

#define NOMINMAX
#include <DirectXTex.h>
#undef NOMINMAX

module Brawler.ModelResolver;
import Brawler.JobGroup;
import Brawler.LaunchParams;
import Util.ModelExport;
import Util.General;
import Brawler.LODScene;
import Brawler.FileMagicHandler;
import Brawler.ByteStream;
import Util.Win32;
import Brawler.Win32.FormattedConsoleMessageBuilder;

#pragma push_macro("AddJob")
#undef AddJob

namespace
{
	static constexpr Brawler::FileMagicHandler MODEL_FILE_MAGIC_HANDLER{ "BMDL" };
	static constexpr std::uint32_t CURRENT_MODEL_FILE_VERSION = 1;

#pragma pack(push)
#pragma pack(1)
	struct CommonModelFileHeader
	{
		std::uint32_t Magic;
		std::uint32_t Version;
	};

	struct VersionedModelFileHeaderV1
	{
		std::uint32_t LODMeshCount;
	};

	using CurrentVersionedModelFileHeader = VersionedModelFileHeaderV1;

	struct CurrentModelFileHeader
	{
		CommonModelFileHeader CommonHeader;
		CurrentVersionedModelFileHeader VersionedHeader;
	};
#pragma pack(pop)

	static constexpr CommonModelFileHeader COMMON_HEADER_VALUE{
		.Magic = MODEL_FILE_MAGIC_HANDLER.GetMagicIntegerValue(),
		.Version = CURRENT_MODEL_FILE_VERSION
	};
}

namespace Brawler
{
	void ModelResolver::Initialize()
	{
		InitializeLODResolvers();
	}

	void ModelResolver::Update()
	{
		Brawler::JobGroup updateGroup{};
		updateGroup.Reserve(mLODResolverPtrArr.size());

		for (const auto& lodResolver : mLODResolverPtrArr)
			updateGroup.AddJob([lodResolverPtr = lodResolver.get()](){ lodResolverPtr->Update(); });

		updateGroup.ExecuteJobs();
	}

	bool ModelResolver::IsReadyForSerialization() const
	{
		// This should not take long to check for each LODResolver instance, so we
		// will just check them all on a single thread.

		for (const auto& lodResolverPtr : mLODResolverPtrArr)
		{
			if (!lodResolverPtr->IsReadyForSerialization())
				return false;
		}

		return true;
	}

	void ModelResolver::SerializeModelData() const
	{
		assert(IsReadyForSerialization());

		assert(mLODResolverPtrArr.size() <= std::numeric_limits<std::uint32_t>::max());
		const CurrentModelFileHeader fileHeader{
			.CommonHeader{COMMON_HEADER_VALUE},
			.VersionedHeader{
				.LODMeshCount = static_cast<std::uint32_t>(mLODResolverPtrArr.size())
			}
		};

		ByteStream modelFileByteStream{};
		modelFileByteStream << fileHeader;

		std::vector<ByteStream> serializedLODMeshByteStreamArr{};
		serializedLODMeshByteStreamArr.resize(mLODResolverPtrArr.size());

		Brawler::JobGroup lodResolverSerializationGroup{};
		lodResolverSerializationGroup.Reserve(mLODResolverPtrArr.size());

		for (const auto i : std::views::iota(0u, mLODResolverPtrArr.size()))
		{
			ByteStream& currByteStream{ serializedLODMeshByteStreamArr[i] };
			LODResolver& currLODResolver{ *(mLODResolverPtrArr[i]) };

			lodResolverSerializationGroup.AddJob([&currByteStream, &currLODResolver] ()
			{
				currByteStream = currLODResolver.GetSerializedLODMeshData();
			});
		}

		lodResolverSerializationGroup.ExecuteJobs();

		for (auto& lodMeshByteStream : serializedLODMeshByteStreamArr)
			modelFileByteStream << lodMeshByteStream;

		const Brawler::LaunchParams& launchParams{ Util::ModelExport::GetLaunchParameters() };

		const std::wstring_view modelName{ launchParams.GetModelName() };
		const std::filesystem::path outputFilePath{ launchParams.GetRootOutputDirectory() / L"Models" / std::filesystem::path{ modelName } / std::format(L"{}.bmdl", modelName) };

		{
			std::error_code errorCode{};

			std::filesystem::create_directories(outputFilePath.parent_path(), errorCode);
			Util::General::CheckErrorCode(errorCode);

			std::ofstream modelFileStream{ outputFilePath, std::ios::out | std::ios::binary };
			modelFileStream << modelFileByteStream;
		}

		Win32::FormattedConsoleMessageBuilder modelExportedMsgBuilder{ Util::Win32::ConsoleFormat::SUCCESS };
		modelExportedMsgBuilder << L"Model Exported: " << Util::Win32::ConsoleFormat::NORMAL << std::format(L"{} -> {}", modelName, outputFilePath.c_str());
		
		modelExportedMsgBuilder.WriteFormattedConsoleMessage();
	}

	void ModelResolver::InitializeLODResolvers()
	{
		const Brawler::LaunchParams& launchParams{ Util::ModelExport::GetLaunchParameters() };
		const std::size_t lodCount = launchParams.GetLODCount();

		Brawler::JobGroup lodResolverCreationGroup{};
		lodResolverCreationGroup.Reserve(lodCount);

		mLODResolverPtrArr.resize(lodCount);

		std::size_t currLOD = 0;
		for (const auto& lodFilePath : launchParams.GetLODFilePaths())
		{
			std::unique_ptr<LODResolver>& lodResolverPtr{ mLODResolverPtrArr[currLOD] };
			lodResolverCreationGroup.AddJob([&lodResolverPtr, currLOD, &lodFilePath] ()
			{
				lodResolverPtr = std::make_unique<LODResolver>(static_cast<std::uint32_t>(currLOD));
				lodResolverPtr->ImportScene();
			});

			++currLOD;
		}

		lodResolverCreationGroup.ExecuteJobs();
	}
}

#pragma pop_macro("AddJob")