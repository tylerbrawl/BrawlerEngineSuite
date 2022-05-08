module;
#include <string>

export module Brawler.I_SourceFileWriter;
import Brawler.FileWriterNode;

export namespace Brawler
{
	namespace SourceFileWriters
	{
		class I_SourceFileWriter
		{
		protected:
			explicit I_SourceFileWriter(std::wstring&& sourceFileName);

		public:
			virtual ~I_SourceFileWriter() = default;

			void WriteSourceFile() const;

		protected:
			virtual Brawler::FileWriterNode CreateFileWriterTree() const = 0;

		private:
			std::wstring mSrcFileName;
		};
	}
}