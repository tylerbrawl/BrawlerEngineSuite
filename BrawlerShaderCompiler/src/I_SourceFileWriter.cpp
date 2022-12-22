module;
#include <string>
#include <fstream>
#include <filesystem>
#include <format>

module Brawler.I_SourceFileWriter;
import Brawler.AppParams;
import Util.General;
import Brawler.ShaderProfileID;
import Util.Win32;

namespace Brawler
{
	namespace SourceFileWriters
	{
		I_SourceFileWriter::I_SourceFileWriter(std::wstring&& sourceFileName) :
			mSrcFileName(std::move(sourceFileName))
		{}

		void I_SourceFileWriter::WriteSourceFile() const
		{
			const Brawler::FileWriterNode rootNode{ CreateFileWriterTree() };

			const std::filesystem::path outputDirectory{ Util::General::GetLaunchParameters().RootSourceDirectory / L"ShaderCompilerFiles" / mSrcFileName };
			
			{
				std::error_code errCode{};
				std::filesystem::create_directories(outputDirectory.parent_path(), errCode);

				if (errCode) [[unlikely]]
					throw std::runtime_error{ std::string{ "ERROR: The directory " } + outputDirectory.string() + " could not be created!" };
			}

			std::ofstream fileStream{ outputDirectory };
			rootNode.WriteOutputText(fileStream);

			// Report that we wrote out this file successfully.
			Util::Win32::WriteFormattedConsoleMessage(std::format(L">{}", mSrcFileName));
		}
	}
}