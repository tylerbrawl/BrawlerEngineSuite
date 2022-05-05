module;
#include <filesystem>
#include <fstream>
#include "DxDef.h"

export module Brawler.BPSWriter;
import Brawler.PipelineEnums;
import Brawler.PSOData;
import Brawler.IMPL.PSODef;
import Brawler.FilePathHash;
import Brawler.IMPL.BPSDef;

export namespace Brawler
{
	template <Brawler::PSOID ID>
	class BPSWriter
	{
	public:
		explicit BPSWriter(const PSOData& psoData);

		BPSWriter(const BPSWriter& rhs) = delete;
		BPSWriter& operator=(const BPSWriter& rhs) = delete;

		BPSWriter(BPSWriter&& rhs) noexcept = default;
		BPSWriter& operator=(BPSWriter&& rhs) noexcept = default;

		void WritePSODataToBPSFile() const;

	private:
		template <typename VersionedBPSHeader>
		void WriteVersionedBPSHeader(std::ofstream& bpsFileStream) const;

		void WritePSOData(std::ofstream& bpsFileStream) const;

	private:
		PSOData* const mPSOData;
	};
}

// ----------------------------------------------------------------------------------

namespace Brawler
{
	template <Brawler::PSOID ID>
	template <typename VersionedBPSHeader>
	void BPSWriter<ID>::WriteVersionedBPSHeader(std::ofstream& bpsFileStream) const
	{
		if constexpr (std::is_same_v<VersionedBPSHeader, Brawler::IMPL::VersionedBPSHeaderV1>)
		{
			ID3DBlob* psoBlobPtr = nullptr;
			CheckHRESULT(mPSOData->PSO->GetCachedBlob(&psoBlobPtr));

			Brawler::IMPL::VersionedBPSHeaderV1 versionedHeader{
				.PSOVersion = Brawler::IMPL::GetCurrentPSOVersionNumber<ID>(),
				.PSOBlobSizeInBytes = psoBlobPtr->GetBufferSize()
			};

			bpsFileStream << versionedHeader;
		}
		else
			static_assert(sizeof(ID) != sizeof(ID), "ERROR: An undefined versioned BPS header format was specified for BPSWriter::WriteVersionedBPSHeader()!");
	}

	template <Brawler::PSOID ID>
	BPSWriter<ID>::BPSWriter(const PSOData& psoData) :
		mPSOData(&psoData)
	{}

	template <Brawler::PSOID ID>
	void BPSWriter<ID>::WritePSODataToBPSFile() const
	{
		constexpr FilePathHash psoNameHash{ Brawler::IMPL::GetPSONameHash<ID>() };
		const std::string psoNameHashStr{ psoNameHash.GetHashString() };

		const std::filesystem::path bpsFilePath{ Brawler::IMPL::PSO_CACHE_PATH / std::filesystem::path{ psoNameHashStr }.replace_extension(Brawler::IMPL::BPS_EXTENSION) };

		std::ofstream bpsFileStream{ bpsFilePath, std::ios_base::out | std::ios_base::binary };

		// Write out the common BPS file header.
		{
			Brawler::IMPL::CommonBPSHeader commonHeader{
				.Magic{ 
					Brawler::IMPL::BPS_MAGIC[0],
					Brawler::IMPL::BPS_MAGIC[1],
					Brawler::IMPL::BPS_MAGIC[2],
					'\0'
				},
				.Version = Brawler::IMPL::CURRENT_BPS_VERSION
			};

			bpsFileStream << commonHeader;
		}

		// Write out the current versioned BPS file header.
		WriteVersionedBPSHeader<Brawler::IMPL::CurrentVersionedBPSHeader>(bpsFileStream);

		// Write out the actual PSO data.
		WritePSOData(bpsFileStream);
	}

	template <Brawler::PSOID ID>
	void BPSWriter<ID>::WritePSOData(std::ofstream& bpsFileStream) const
	{
		ID3DBlob* psoBlobPtr = nullptr;
		CheckHRESULT(mPSOData->PSO->GetCachedBlob(&psoBlobPtr));

		bpsFileStream.write(reinterpret_cast<const char*>(psoBlobPtr->GetBufferPointer()), psoBlobPtr->GetBufferSize());
	}
}