module;

export module Brawler.PSODefinitionBaseFileWriter;
import Brawler.I_SourceFileWriter;
import Brawler.FileWriterNode;

export namespace Brawler
{
	namespace SourceFileWriters
	{
		class PSODefinitionsBaseFileWriter final : public I_SourceFileWriter
		{
		public:
			PSODefinitionsBaseFileWriter();

			PSODefinitionsBaseFileWriter(const PSODefinitionsBaseFileWriter& rhs) = delete;
			PSODefinitionsBaseFileWriter& operator=(const PSODefinitionsBaseFileWriter& rhs) = delete;

			PSODefinitionsBaseFileWriter(PSODefinitionsBaseFileWriter&& rhs) noexcept = default;
			PSODefinitionsBaseFileWriter& operator=(PSODefinitionsBaseFileWriter&& rhs) noexcept = default;

		protected:
			Brawler::FileWriterNode CreateFileWriterTree() const override;
		};
	}
}