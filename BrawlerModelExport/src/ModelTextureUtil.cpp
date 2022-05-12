module;
#include <array>
#include <string>
#include <filesystem>
#include <fstream>
#include <DirectXTex.h>
#include "DxDef.h"

module Util.ModelTexture;
import Brawler.FileAttributes;

namespace Util
{
	namespace ModelTexture
	{
		void WriteTextureToFile(const TextureWriteInfo& writeInfo)
		{
			std::error_code errorCode{};
			std::filesystem::create_directories(writeInfo.OutputDirectory.parent_path(), errorCode);

			if (errorCode) [[unlikely]]
				throw std::runtime_error{ std::string{ "ERROR: Util::ModelTexture::WriteTextureToFile() failed to create a texture's parent directory with the following error message: "} + errorCode.message() };

			std::ofstream fileStream{ writeInfo.OutputDirectory, std::ios::binary | std::ios::out };

			constexpr Brawler::FileAttributes::BTEX::CommonBTEXHeader commonHeader{ Brawler::FileAttributes::BTEX::CreateCommonHeader() };
			fileStream << commonHeader;

			const Brawler::FileAttributes::BTEX::CurrentVersionedBTEXHeader versionedHeader{
				.DDSFileSize = writeInfo.DDSBlob.GetBufferSize()
			};
			fileStream << versionedHeader;

			// Write out the resource description.
			fileStream.write(reinterpret_cast<const char*>(&(writeInfo.ResourceDescription)), sizeof(writeInfo.ResourceDescription));

			// Write out the DDS data.
			fileStream.write(reinterpret_cast<const char*>(writeInfo.DDSBlob.GetBufferPointer()), writeInfo.DDSBlob.GetBufferSize());
		}
	}
}