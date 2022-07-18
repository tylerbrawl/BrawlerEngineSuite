module;
#include <memory>
#include <vector>
#include <span>
#include <cassert>
#include <ranges>

module Brawler.Model;
import Brawler.MeshTypeID;
import Brawler.FileMagicHandler;
import Brawler.AssetManagement.BPKArchiveReader;
import Brawler.MappedFileView;
import Brawler.FileAccessMode;
import Util.General;
import Brawler.SerializedStruct;
import Brawler.JobSystem;
import Brawler.StaticLODMeshDefinition;

namespace
{
	static constexpr Brawler::FileMagicHandler MODEL_FILE_MAGIC_HANDLER{ "BMDL" };
	static constexpr std::uint32_t CURRENT_MODEL_FILE_VERSION = 1;

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

	static constexpr CommonModelFileHeader EXPECTED_COMMON_HEADER_VALUE{
		.Magic = MODEL_FILE_MAGIC_HANDLER.GetMagicIntegerValue(),
		.Version = CURRENT_MODEL_FILE_VERSION
	};

	struct LODMeshTOCEntry
	{
		std::uint64_t OffsetFromFileStart;
		Brawler::MeshTypeID MeshType;
	};

	bool IsModelFileCommonHeaderValid(const Brawler::MappedFileView<FileAccessMode::READ_ONLY>& bmdlFileMappedView)
	{
		const std::span<const std::byte> mappedDataSpan{ bmdlFileMappedView.GetMappedData() };

		if (mappedDataSpan.size_bytes() < sizeof(CurrentModelFileHeader)) [[unlikely]]
			return false;
		
		Brawler::SerializedStruct<CommonModelFileHeader> serializedCommonHeader{};
		std::memcpy(&serializedCommonHeader, mappedDataSpan.data(), sizeof(serializedCommonHeader));

		const CommonModelFileHeader deserializedCommonHeader{ Brawler::DeserializeData(serializedCommonHeader) };

		// Ensure that the magic matches.
		if (!MODEL_FILE_MAGIC_HANDLER.DoesMagicIntegerMatch(deserializedCommonHeader.Magic)) [[unlikely]]
			return false;

		// Only allow model files with the current version.
		return (deserializedCommonHeader.Version == CURRENT_MODEL_FILE_VERSION);
	}

	struct LODMeshDefinitionCreationParams
	{
		Brawler::FilePathHash BMDLFileHash;
		std::uint32_t LODMeshID;
		Brawler::MeshTypeID TypeID;
	};

	std::unique_ptr<Brawler::I_LODMeshDefinition> CreateLODMeshDefinition(const LODMeshDefinitionCreationParams& params)
	{
		switch (params.TypeID)
		{
		case Brawler::MeshTypeID::STATIC:
			return std::make_unique<Brawler::StaticLODMeshDefinition>(params.BMDLFileHash, params.LODMeshID);

		// NOTE: Skinned meshes are currently unsupported.

		default: [[unlikely]]
		{
			assert(false && "ERROR: An unrecognized mesh type was detected in a .bmdl file! (If a derived I_LODMeshDefinition class exists for it, then please add it to <anonymous namespace>::CreateLODMeshDefinition() in Model.cpp.)");
			std::unreachable();

			return nullptr;
		}
		}
	}
}

namespace Brawler
{
	Model::Model(const FilePathHash bmdlFileHash) :
		mLODMeshDefinitionPtrArr()
	{
		CreateLODMeshDefinitions(bmdlFileHash);
	}

	void Model::CreateLODMeshDefinitions(const FilePathHash bmdlFileHash)
	{
		// Before we can create the LODMeshDefinition instances, we need to know what type
		// of mesh each LOD mesh is. To do that, we analyze the .bmdl file partially and then
		// pass off the rest to the LODMeshDefinition instances.

		// Since .bmdl files are expected to be small, we disable compression for them.
		if constexpr (Util::General::IsDebugModeEnabled())
		{
			const AssetManagement::BPKArchiveReader::TOCEntry& bmdlFileTOCEntry{ AssetManagement::BPKArchiveReader::GetInstance().GetTableOfContentsEntry(bmdlFileHash) };
			assert(!bmdlFileTOCEntry.IsDataCompressed() && "ERROR: A compressed .bmdl file was detected in the BPK archive!");
		}

		const MappedFileView<FileAccessMode::READ_ONLY> bmdlFileMappedView{ AssetManagement::BPKArchiveReader::GetInstance().CreateMappedFileViewForAsset(bmdlFileHash) };
		assert(bmdlFileMappedView.IsValidView());

		assert(IsModelFileCommonHeaderValid(bmdlFileMappedView) && "ERROR: An invalid BMDL file was detected within the BPK archive!");

		const std::span<const std::byte> mappedDataSpan{ bmdlFileMappedView.GetMappedData() };
		CurrentVersionedModelFileHeader versionedHeader{};

		{
			SerializedStruct<CurrentVersionedModelFileHeader> serializedVersionedHeader{};
			std::memcpy(&serializedVersionedHeader, (mappedDataSpan.data() + sizeof(CommonModelFileHeader)), sizeof(serializedVersionedHeader));

			versionedHeader = DeserializeData(serializedVersionedHeader);
		}

		mLODMeshDefinitionPtrArr.resize(versionedHeader.LODMeshCount);

		Brawler::JobGroup lodMeshDefinitionGroup{};
		lodMeshDefinitionGroup.Reserve(versionedHeader.LODMeshCount);

		std::vector<SerializedStruct<LODMeshTOCEntry>> serializedTOCEntryArr{};
		serializedTOCEntryArr.resize(versionedHeader.LODMeshCount);

		{
			const std::span<SerializedStruct<LODMeshTOCEntry>> serializedTOCEntrySpan{ serializedTOCEntryArr };
			assert(sizeof(CurrentModelFileHeader) + serializedTOCEntrySpan.size_bytes() <= mappedDataSpan.size_bytes());

			std::memcpy(serializedTOCEntrySpan.data(), (mappedDataSpan.data() + sizeof(CurrentModelFileHeader)), serializedTOCEntrySpan.size_bytes());
		}
		
		for (const auto i : std::views::iota(0u, versionedHeader.LODMeshCount))
		{
			std::unique_ptr<I_LODMeshDefinition>& currDefinitionPtr{ mLODMeshDefinitionPtrArr[i] };
			const LODMeshDefinitionCreationParams creationParams{
				.BMDLFileHash = bmdlFileHash,
				.LODMeshID = i,
				.TypeID = serializedTOCEntryArr[i].MeshType
			};

			lodMeshDefinitionGroup.AddJob([&currDefinitionPtr, creationParams] ()
			{
				currDefinitionPtr = CreateLODMeshDefinition(creationParams);
			});
		}

		lodMeshDefinitionGroup.ExecuteJobs();
	}
}