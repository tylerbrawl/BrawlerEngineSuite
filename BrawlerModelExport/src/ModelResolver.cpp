module;
#include <memory>
#include <vector>
#include <ranges>
#include <cassert>
#include <format>
#include <filesystem>
#include <fstream>
#include <atomic>

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
import Brawler.BCAInfoEntry;
import Brawler.BCAInfoWriter;
import Brawler.MeshTypeID;

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

	void CreateBCAInfoFileForVirtualTextures()
	{
		// As of writing this, virtual texture page data is already compressed with zstandard when it
		// gets written out to the file system (see VirtualTextureCPUPageStore::CompressVirtualTexturePage()),
		// so we don't want the Brawler File Packer to compress it again. To do that, we need to explicitly
		// disable compression with a .BCAINFO file.
		//
		// To do this, we are going to search the output texture directory and add a "DoNotCompress" entry for
		// each file we find. Theoretically, we could make this a member function of ModelResolver and
		// have the LODResolver instances provide us with a list of output texture files. However, since the
		// BrawlerFilePacker always blindly copies all of the files in a directory into the .bpk archive
		// (excluding the .BCAINFO file), I don't see doing this as being worth the added complexity.

		const Brawler::LaunchParams& launchParams{ Util::ModelExport::GetLaunchParameters() };
		const std::filesystem::path completeOutputDirectoryPath{ launchParams.GetRootOutputDirectory() / L"Textures" / launchParams.GetModelName() };

		// Delete any existing .BCAINFO file, should one exist.
		{
			const std::filesystem::path completeBCAInfoOutputPath{ completeOutputDirectoryPath / L".BCAINFO" };
			std::filesystem::remove(completeBCAInfoOutputPath);
		}

		Brawler::BCAInfoWriter bcaInfoWriter{};

		for (const auto& filePath : std::filesystem::directory_iterator{ completeOutputDirectoryPath })
		{
			// std::filesystem::directory_iterator only returns const& values, so we have to create a
			// copy.
			Brawler::BCAInfoEntry currEntry{ std::filesystem::path{ filePath } };
			currEntry.SetCompressionStatus(false);

			bcaInfoWriter.AddEntry(std::move(currEntry));
		}

		bcaInfoWriter.SerializeBCAInfoFile(completeOutputDirectoryPath);
	}

	void CreateBCAInfoFileForBMDLFile()
	{
		const Brawler::LaunchParams& launchParams{ Util::ModelExport::GetLaunchParameters() };
		const std::filesystem::path completeOutputDirectoryPath{ launchParams.GetRootOutputDirectory() / L"Models" / launchParams.GetModelName() };

		// Delete any existing .BCAINFO file, should one exist.
		{
			const std::filesystem::path completeBCAInfoOutputPath{ completeOutputDirectoryPath / L".BCAINFO" };
			std::filesystem::remove(completeBCAInfoOutputPath);
		}

		Brawler::BCAInfoWriter bcaInfoWriter{};

		Brawler::BCAInfoEntry bmdlFileEntry{ completeOutputDirectoryPath / std::format(L"{}.bmdl", launchParams.GetModelName()) };
		bmdlFileEntry.SetCompressionStatus(false);

		bcaInfoWriter.AddEntry(std::move(bmdlFileEntry));
		bcaInfoWriter.SerializeBCAInfoFile(completeOutputDirectoryPath);
	}
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

		// After we have gotten all of the serialized mesh data, we know that the texture data
		// has also been written out; this is because the I_MaterialDefinition instance of a
		// MeshResolver is responsible for serializing the data.
		//
		// As of writing this, virtual texture data is already compressed with zstandard, so
		// we don't want the BrawlerFilePacker to run compression again. To prevent that from
		// happening, we create a .BCAINFO file in the same directory as the model texture
		// data which specifies to disable compression for every texture file contained within
		// it.
		// 
		// In addition, we also want to create a .BCAINFO file to disable compression for the
		// .bmdl file which we are generating. These files contain the metadata needed to do
		// further asset loading, and are typically small enough that compression does not help
		// much.
		//
		// To increase parallelism, we can do both of these tasks concurrently with the file I/O 
		// for the .bmdl file.
		Brawler::JobGroup postModelSerializationGroup{};
		postModelSerializationGroup.Reserve(2);

		std::atomic<std::uint64_t> bcaInfoCreationCompletedCount{ 2 };

		postModelSerializationGroup.AddJob([&bcaInfoCreationCompletedCount] ()
		{
			CreateBCAInfoFileForVirtualTextures();

			bcaInfoCreationCompletedCount.fetch_sub(1, std::memory_order::relaxed);
			bcaInfoCreationCompletedCount.notify_one();
		});

		postModelSerializationGroup.AddJob([&bcaInfoCreationCompletedCount] ()
		{
			CreateBCAInfoFileForBMDLFile();

			bcaInfoCreationCompletedCount.fetch_sub(1, std::memory_order::relaxed);
			bcaInfoCreationCompletedCount.notify_one();
		});

		postModelSerializationGroup.ExecuteJobsAsync();

		// Before we add the actual LOD mesh data to the .bmdl file, we should specify both the file
		// offsets for each definition and the MeshTypeID of the LOD mesh. That way, they can be quickly 
		// loaded at runtime.
		std::uint64_t currLODMeshDataOffset = modelFileByteStream.GetByteCount() + ((sizeof(std::uint64_t) + sizeof(MeshTypeID)) * serializedLODMeshByteStreamArr.size());

		for (const auto i : std::views::iota(0u, mLODResolverPtrArr.size()))
		{
			const MeshTypeID lodResolverMeshType{ mLODResolverPtrArr[i]->GetMeshTypeID() };
			assert(lodResolverMeshType != MeshTypeID::COUNT_OR_ERROR);

			modelFileByteStream << currLODMeshDataOffset << std::to_underlying(lodResolverMeshType);
			currLODMeshDataOffset += serializedLODMeshByteStreamArr[i].GetByteCount();
		}

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

		// Wait to report that we are finished until after the .BCAINFO files could be created.
		// (Oh, and we also don't want to exit this function until after that, either.)
		std::uint64_t expectedBCAInfoCompletionCountValue = 2;
		while (expectedBCAInfoCompletionCountValue != 0)
		{
			bcaInfoCreationCompletedCount.wait(expectedBCAInfoCompletionCountValue, std::memory_order::relaxed);
			expectedBCAInfoCompletionCountValue = bcaInfoCreationCompletedCount.load(std::memory_order::relaxed);
		}
		
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