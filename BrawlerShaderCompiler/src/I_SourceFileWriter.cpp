module;
#include <string>
#include <fstream>
#include <filesystem>

module Brawler.I_SourceFileWriter;
import Brawler.AppParams;
import Util.General;
import Brawler.ShaderProfileID;

namespace Brawler
{
	namespace SourceFileWriters
	{
		I_SourceFileWriter::I_SourceFileWriter(const std::wstring_view sourceFileName) :
			mSrcFileName(sourceFileName)
		{}

		void I_SourceFileWriter::WriteSourceFile() const
		{
			const Brawler::FileWriterNode rootNode{ CreateFileWriterTree() };

			const std::filesystem::path outputDirectory{ Util::General::GetLaunchParameters().RootSourceDirectory / mSrcFileName.data() };

			std::ofstream fileStream{ outputDirectory };
			rootNode.WriteOutputText(fileStream);
		}
	}
}