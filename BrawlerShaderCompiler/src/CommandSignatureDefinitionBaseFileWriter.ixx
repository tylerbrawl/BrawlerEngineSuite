module;

export module Brawler.CommandSignatureDefinitionBaseFileWriter;
import Brawler.I_SourceFileWriter;
import Brawler.FileWriterNode;

export namespace Brawler
{
	namespace SourceFileWriters
	{
		class CommandSignatureDefinitionBaseFileWriter final : public I_SourceFileWriter
		{
		public:
			CommandSignatureDefinitionBaseFileWriter();

			CommandSignatureDefinitionBaseFileWriter(const CommandSignatureDefinitionBaseFileWriter& rhs) = delete;
			CommandSignatureDefinitionBaseFileWriter& operator=(const CommandSignatureDefinitionBaseFileWriter& rhs) = delete;

			CommandSignatureDefinitionBaseFileWriter(CommandSignatureDefinitionBaseFileWriter&& rhs) noexcept = default;
			CommandSignatureDefinitionBaseFileWriter& operator=(CommandSignatureDefinitionBaseFileWriter&& rhs) noexcept = default;

		protected:
			Brawler::FileWriterNode CreateFileWriterTree() const override;
		};
	}
}