module;
#include <cstdint>
#include <vector>
#include <ranges>
#include <unordered_map>
#include <mutex>
#include <filesystem>
#include <fstream>
#include <DxDef.h>
#include <DirectXTex.h>

module Brawler.ModelTextureSerializer;
import Brawler.FileMagicHandler;
import Util.General;
import Util.Engine;
import Util.ModelExport;
import Brawler.SHA512Hashing;
import Brawler.LaunchParams;

namespace
{
	static constexpr Brawler::FileMagicHandler MODEL_TEXTURE_MAGIC_HANDLER{ "BMTX" };
	static constexpr std::uint32_t CURRENT_MODEL_TEXTURE_FORMAT_VERSION = 1;

#pragma pack(push)
#pragma pack(1)
	struct CommonModelTextureHeader
	{
		std::uint32_t Magic;
		std::uint32_t Version;
	};

	struct VersionedModelTextureHeaderV1
	{
		Brawler::D3D12_RESOURCE_DESC ResourceDescription;
	};

	using CurrentVersionedModelTextureHeader = VersionedModelTextureHeaderV1;

	struct CurrentModelTextureHeader
	{
		CommonModelTextureHeader CommonHeader;
		CurrentVersionedModelTextureHeader VersionedHeader;
	};
#pragma pack(pop)

	static constexpr CommonModelTextureHeader COMMON_HEADER_VALUE{
		.Magic = MODEL_TEXTURE_MAGIC_HANDLER.GetMagicIntegerValue(),
		.Version = CURRENT_MODEL_TEXTURE_FORMAT_VERSION
	};
}

namespace
{
	Brawler::D3D12_RESOURCE_DESC CreateResourceDescriptionFromTexMetadata(const DirectX::TexMetadata& metadata)
	{
		return Brawler::D3D12_RESOURCE_DESC{
			.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension),
			.Alignment = 0,
			.Width = metadata.width,
			.Height = static_cast<std::uint32_t>(metadata.height),
			.DepthOrArraySize = (metadata.dimension == DirectX::TEX_DIMENSION::TEX_DIMENSION_TEXTURE3D ? static_cast<std::uint16_t>(metadata.depth) : static_cast<std::uint16_t>(metadata.arraySize)),
			.MipLevels = static_cast<std::uint16_t>(metadata.mipLevels),
			.Format = metadata.format,
			.SampleDesc{
				.Count = 1,
				.Quality = 0
			},
			.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN,
			.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE,
			.SamplerFeedbackMipRegion{}
		};
	}

	std::vector<std::byte> CreateModelTextureFileDataByteArray(const DirectX::ScratchImage& finalizedModelTexture)
	{
		// Prepare the texture data for upload to the GPU. This data can be sent directly to DirectStorage
		// or straight into a D3D12Resource texture.
		std::vector<D3D12_SUBRESOURCE_DATA> subResourceDataArr{};
		const DirectX::TexMetadata& modelTextureMetadata{ finalizedModelTexture.GetMetadata() };

		Util::General::CheckHRESULT(DirectX::PrepareUpload(
			&(Util::Engine::GetD3D12Device()),
			finalizedModelTexture.GetImages(),
			finalizedModelTexture.GetImageCount(),
			modelTextureMetadata,
			subResourceDataArr
		));

		const CurrentModelTextureHeader fileHeader{
			.CommonHeader{ COMMON_HEADER_VALUE },
			.VersionedHeader{
				.ResourceDescription{ CreateResourceDescriptionFromTexMetadata(modelTextureMetadata) }
			}
		};

		const std::uint32_t numSubResources = [&modelTextureMetadata] ()
		{
			const std::uint32_t formatPlaneCount = D3D12GetFormatPlaneCount(&(Util::Engine::GetD3D12Device()), modelTextureMetadata.format);
			const std::uint32_t maxIndexValue = D3D12CalcSubresource(
				static_cast<std::uint32_t>(modelTextureMetadata.mipLevels - 1),
				(modelTextureMetadata.dimension == DirectX::TEX_DIMENSION::TEX_DIMENSION_TEXTURE3D ? 0 : static_cast<std::uint32_t>(modelTextureMetadata.arraySize - 1)),
				(formatPlaneCount - 1),
				static_cast<std::uint32_t>(modelTextureMetadata.mipLevels),
				static_cast<std::uint32_t>(modelTextureMetadata.arraySize)
			);

			return (maxIndexValue + 1);
		}();

		assert(numSubResources == static_cast<std::uint32_t>(modelTextureMetadata.mipLevels));

		std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> subResourceFootprintArr{};
		subResourceFootprintArr.resize(numSubResources);

		std::vector<std::uint32_t> subResourceRowCountArr{};
		subResourceRowCountArr.resize(numSubResources);

		std::vector<std::uint64_t> unpaddedSubResourceRowSizeArr{};
		unpaddedSubResourceRowSizeArr.resize(numSubResources);

		std::uint64_t totalResourceSizeInBytes = 0;

		Util::Engine::GetD3D12Device().GetCopyableFootprints1(
			&(fileHeader.VersionedHeader.ResourceDescription),
			0,
			numSubResources,
			0,
			subResourceFootprintArr.data(),
			subResourceRowCountArr.data(),
			unpaddedSubResourceRowSizeArr.data(),
			&totalResourceSizeInBytes
		);

		std::vector<std::uint64_t> subResourceOffsetArr{};
		subResourceOffsetArr.resize(numSubResources);

		const std::size_t baseSubResourceOffset = sizeof(CurrentModelTextureHeader) + (numSubResources * sizeof(std::uint64_t));

		std::vector<std::byte> resourceDataByteArr{};
		resourceDataByteArr.resize(totalResourceSizeInBytes);

		for (const auto i : std::views::iota(0u, numSubResources))
		{
			const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& currSubResourceFootprint{ subResourceFootprintArr[i] };
			
			// For each sub-resource, write out the offset from the start of the model texture file
			// at which its data can be found.
			subResourceOffsetArr[i] = baseSubResourceOffset + currSubResourceFootprint.Offset;

			const D3D12_MEMCPY_DEST copyDest{
				.pData = (resourceDataByteArr.data() + currSubResourceFootprint.Offset),
				.RowPitch = currSubResourceFootprint.Footprint.RowPitch,
				.SlicePitch = (currSubResourceFootprint.Footprint.RowPitch * subResourceRowCountArr[i])
			};

			const D3D12_SUBRESOURCE_DATA& copySrc{ subResourceDataArr[i] };

			MemcpySubresource(
				&copyDest,
				&copySrc,
				unpaddedSubResourceRowSizeArr[i],
				subResourceRowCountArr[i],
				currSubResourceFootprint.Footprint.Depth
			);
		}

		std::vector<std::byte> modelTextureFileByteArr{};

		const auto headerDataSpan{ std::as_bytes(std::span<const CurrentModelTextureHeader>{ &fileHeader, 1 }) };
		const auto subResourceOffsetDataSpan{ std::as_bytes(std::span<const std::uint64_t>{ subResourceOffsetArr }) };
		const auto resourceDataSpan{ std::span<const std::byte>{ resourceDataByteArr } };

		modelTextureFileByteArr.resize(headerDataSpan.size_bytes() + subResourceOffsetDataSpan.size_bytes() + resourceDataSpan.size_bytes());
		std::size_t currDataSize = 0;

		std::memcpy(modelTextureFileByteArr.data(), headerDataSpan.data(), headerDataSpan.size_bytes());
		currDataSize += headerDataSpan.size_bytes();

		std::memcpy(modelTextureFileByteArr.data() + currDataSize, subResourceOffsetDataSpan.data(), subResourceOffsetDataSpan.size_bytes());
		currDataSize += subResourceOffsetDataSpan.size_bytes();

		std::memcpy(modelTextureFileByteArr.data() + currDataSize, resourceDataSpan.data(), resourceDataSpan.size_bytes());

		return modelTextureFileByteArr;
	}
}

namespace
{
	static std::unordered_map<Brawler::SHA512Hash, std::size_t> modelTextureFilePathHashMap{};
	static std::mutex modelTextureFilePathHashMapCriticalSection{};
}

namespace Brawler
{
	FilePathHash SerializeModelTexture(const NZWStringView textureName, const DirectX::ScratchImage& finalizedModelTexture)
	{
		const std::vector<std::byte> modelTextureFileByteArr{ CreateModelTextureFileDataByteArray(finalizedModelTexture) };
		const SHA512Hash modelTextureFileHash{ [&modelTextureFileByteArr]()
		{
			SHA512Hasher shaHasher{};
			return shaHasher.HashData(std::span<const std::byte>{modelTextureFileByteArr});
		}() };

		// Create the std::filesystem::path object here, even if we don't actually end up writing anything out
		// to the file system. That way, we spend less time in the std::mutex.
		const Brawler::LaunchParams& launchParams{ Util::ModelExport::GetLaunchParameters() };
		std::filesystem::path outputFilePathSubDirectory{ L"Textures" / std::filesystem::path{ launchParams.GetModelName() } / textureName.C_Str() };
		outputFilePathSubDirectory.replace_extension(L".bmtex");

		// By convention, all file path hashes in the Brawler Engine are to be constructed such that the root data
		// directory is *NOT* included in the hash calculation. For instance, if the full file path for a data file
		// is C:\BrawlerEngineData\Textures\ModelName\DiffuseAlbedo.bmtex and the root data directory is
		// C:\BrawlerEngineData, then only Textures\ModelName\DiffuseAlbedo.bmtex will be used when creating the hash.
		FilePathHash texturePathHash{ outputFilePathSubDirectory.c_str() };

		const std::filesystem::path fullOutputPath{ launchParams.GetRootOutputDirectory() / outputFilePathSubDirectory };
		bool shouldWriteFile = false;

		// Here, we attempt to prevent creating duplicate textures in the file system. For each resolved model texture,
		// we compare its SHA-512 hash to that of all previously created SHA-512 hash values. If we find a texture with
		// the same hash, then we return the FilePathHash created for that model texture and write nothing to the file
		// system.
		//
		// This check is conservative and not exhaustive. For instance, it may so happen that two model textures get
		// created which are almost the exact same; the one difference is that they have a differing number of mip levels.
		// This check will fail to detect the similarity, and will result in a redundant copy of the texture data.
		{
			std::scoped_lock<std::mutex> lock{ modelTextureFilePathHashMapCriticalSection };

			if (modelTextureFilePathHashMap.contains(modelTextureFileHash)) [[unlikely]]
				texturePathHash = FilePathHash{ modelTextureFilePathHashMap.at(modelTextureFileHash) };

			else [[likely]]
			{
				modelTextureFilePathHashMap[modelTextureFileHash] = texturePathHash.GetHash();
				shouldWriteFile = true;
			}
		}

		if (!shouldWriteFile) [[unlikely]]
			return texturePathHash;

		// If this texture data is unique, then we write it out to the file system.
		std::error_code errorCode{};

		std::filesystem::create_directories(fullOutputPath.parent_path(), errorCode);
		Util::General::CheckErrorCode(errorCode);

		std::ofstream modelTextureFileStream{ fullOutputPath, std::ios::out | std::ios::binary };
		modelTextureFileStream.write(reinterpret_cast<const char*>(modelTextureFileByteArr.data()), modelTextureFileByteArr.size());

		return texturePathHash;
	}
}