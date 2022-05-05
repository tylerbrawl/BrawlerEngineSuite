module;
#include <optional>
#include <filesystem>
#include <fstream>
#include <array>
#include "DxDef.h"

export module Brawler.BPSReader;
import Brawler.PipelineEnums;
import Brawler.IMPL.BPSDef;
import Brawler.FilePathHash;
import Brawler.IMPL.PSODef;

export namespace Brawler
{
	template <Brawler::PSOID ID>
	class BPSReader
	{
	public:
		BPSReader();

		BPSReader(const BPSReader& rhs) = delete;
		BPSReader& operator=(const BPSReader& rhs) = delete;

		BPSReader(BPSReader&& rhs) noexcept = default;
		BPSReader& operator=(BPSReader&& rhs) noexcept = default;

		void AttemptPSOBlobExtraction();
		Microsoft::WRL::ComPtr<ID3DBlob> GetCachedPSOBlob();
		bool IsBPSFileOutdated() const;

	private:
		std::optional<std::ifstream> CreateBPSFileStream() const;
		void GetPSODataFromBPSFileStream(std::ifstream& bpsFileStream);

		template <typename VersionedBPSHeader>
		Microsoft::WRL::ComPtr<ID3DBlob> ExtractCachedPSOBlob(std::ifstream& bpsFileStream) const;

	private:
		Microsoft::WRL::ComPtr<ID3DBlob> mCachedPSOBlob;
		std::uint32_t mBPSFileVersion;
	};
}

// -------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <Brawler::PSOID ID>
	template <typename VersionedBPSHeader>
	Microsoft::WRL::ComPtr<ID3DBlob> BPSReader<ID>::ExtractCachedPSOBlob(std::ifstream& bpsFileStream) const
	{
		if constexpr (std::is_same_v<VersionedBPSHeader, Brawler::IMPL::VersionedBPSHeaderV1>)
		{
			Brawler::IMPL::VersionedBPSHeaderV1 versionedHeader{};
			bpsFileStream >> versionedHeader;

			// If the PSO version number is not the same as the current PSO version number,
			// then reject the cached PSO.
			if (versionedHeader.PSOVersion != Brawler::IMPL::GetCurrentPSOVersionNumber<ID>()) [[unlikely]]
				return nullptr;

			Microsoft::WRL::ComPtr<ID3DBlob> cachedPSOBlob{ nullptr };
			CheckHRESULT(D3DCreateBlob(versionedHeader.PSOBlobSizeInBytes, &cachedPSOBlob));

			// Read the cached PSO data directly into the ID3DBlob.
			bpsFileStream.read(reinterpret_cast<char*>(cachedPSOBlob->GetBufferPointer()), cachedPSOBlob->GetBufferSize());

			return cachedPSOBlob;
		}
		else
		{
			static_assert(sizeof(ID) != sizeof(ID), "ERROR: An undefined versioned BPS header format was specified for BPSReader::ExtractCachedPSOBlob()!");
			return nullptr;
		}
		
	}

	template <Brawler::PSOID ID>
	BPSReader<ID>::BPSReader() :
		mCachedPSOBlob(),
		mBPSFileVersion(0)
	{}

	template <Brawler::PSOID ID>
	void BPSReader<ID>::AttemptPSOBlobExtraction()
	{
		std::optional<std::ifstream> bpsFileStream{ CreateBPSFileStream() };

		if (bpsFileStream.has_value()) [[likely]]
			GetPSODataFromBPSFileStream(*bpsFileStream);
	}

	template <Brawler::PSOID ID>
	Microsoft::WRL::ComPtr<ID3DBlob> BPSReader<ID>::GetCachedPSOBlob()
	{
		return mCachedPSOBlob;
	}

	template <Brawler::PSOID ID>
	bool BPSReader<ID>::IsBPSFileOutdated() const
	{
		return (mBPSFileVersion != Brawler::IMPL::CURRENT_BPS_VERSION);
	}

	template <Brawler::PSOID ID>
	std::optional<std::ifstream> BPSReader<ID>::CreateBPSFileStream() const
	{
		// First, make sure that the parent directory exists. There is technically a race
		// condition here between threads, but since std::filesystem::create_directories()
		// does nothing and does not return an error code if the path already exists, it
		// is benign.
		std::error_code parentDirCreationError{};
		std::filesystem::create_directories(Brawler::IMPL::PSO_CACHE_PATH, parentDirCreationError);

		if (parentDirCreationError) [[unlikely]]
			throw std::runtime_error{ "ERROR: Creating the PSO Cache directory failed with the following error: " + parentDirCreationError.message() };

		constexpr FilePathHash psoNameHash{ Brawler::IMPL::GetPSONameHash<ID>() };
		const std::string psoNameHashStr{ psoNameHash.GetHashString() };

		const std::filesystem::path cachedPSOPath{ Brawler::IMPL::PSO_CACHE_PATH / std::filesystem::path{psoNameHashStr}.replace_extension(Brawler::IMPL::BPS_EXTENSION) };
		if (!std::filesystem::exists(cachedPSOPath) || !std::filesystem::is_directory(cachedPSOPath)) [[unlikely]]
			return std::optional<std::ifstream>{};

		return std::optional<std::ifstream>{ std::in_place_t, cachedPSOPath, std::ios_base::in | std::ios_base::binary };
	}

	template <Brawler::PSOID ID>
	void BPSReader<ID>::GetPSODataFromBPSFileStream(std::ifstream& bpsFileStream)
	{
		// Ensure the validity of the .bps file.
		Brawler::IMPL::CommonBPSHeader commonHeader{};
		bpsFileStream >> commonHeader;

		for (std::size_t i = 0; i < Brawler::IMPL::BPS_MAGIC.size(); ++i)
		{
			if (commonHeader.Magic[i] != Brawler::IMPL::BPS_MAGIC[i]) [[unlikely]]
				return;
		}

		// Store the version of the BPS file format here. That way, we can re-create it if
		// we want to use a new format.
		mBPSFileVersion = commonHeader.Version;

		// Extract the cached PSO data based on the specified BPS version number.
		switch (commonHeader.Version)
		{
		case Brawler::IMPL::CURRENT_BPS_VERSION: [[likely]]
		{
			mCachedPSOBlob = ExtractCachedPSOBlob<Brawler::IMPL::CurrentVersionedBPSHeader>(bpsFileStream);
			break;
		}

			// Add *ALL* cases (even currently redundant ones, i.e., the case has the same current 
			// switch value as Brawler::IMPL::CURRENT_BPS_VERSION) here.

		case 1:
		{
			mCachedPSOBlob = ExtractCachedPSOBlob<Brawler::IMPL::VersionedBPSHeaderV1>(bpsFileStream);
			break;
		}

		default: [[unlikely]]
		{
			// If we get here, then either this BPS version is unrecognized, or the
			// file was corrupted/edited/etc. Regardless of the cause, we cannot use
			// this file to get the cached PSO data.
			break;
		}
		}
	}
}