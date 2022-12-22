module;
#include <string_view>

module Brawler.CommandSignatureDefinitionBaseFileWriter;
import Brawler.FileStrings;

/*
Source File Name: CommandSignatureDefinitionBase.ixx

Contents:

// WARNING: This file was auto-generated by the Brawler Shader Compiler. You will incur the
// wrath of God if you dare touch it.

module;

export module Brawler.CommandSignatures.CommandSignatureDefinition:CommandSignatureDefinitionBase;
import Brawler.CommandSignatures.CommandSignatureID;

export namespace Brawler
{
	namespace CommandSignatures
	{
		template <CommandSignatureID CSIdentifier>
		struct CommandSignatureDefinition
		{};
	}
}
*/

namespace Brawler
{
	namespace SourceFileWriters
	{
		CommandSignatureDefinitionBaseFileWriter::CommandSignatureDefinitionBaseFileWriter() :
			I_SourceFileWriter(L"CommandSignatureDefinitionBase.ixx")
		{}

		Brawler::FileWriterNode CommandSignatureDefinitionBaseFileWriter::CreateFileWriterTree() const
		{
			FileWriterNode rootNode{};

			{
				FileWriterNode autoGeneratedWarningNode{};
				autoGeneratedWarningNode.SetOutputText(std::string{ Brawler::FileStrings::AUTO_GENERATED_WARNING_COMMENT });

				rootNode.AddChildNode(std::move(autoGeneratedWarningNode));
			}

			{
				static constexpr std::string_view FILE_CONTENTS_STRING{
R"(module;

export module Brawler.CommandSignatures.CommandSignatureDefinition:CommandSignatureDefinitionBase;
import Brawler.CommandSignatures.CommandSignatureID;

export namespace Brawler
{
	namespace CommandSignatures
	{
		template <CommandSignatureID CSIdentifier>
		struct CommandSignatureDefinition
		{};
	}
})"
				};

				FileWriterNode fileContentsNode{};
				fileContentsNode.SetOutputText(std::string{ FILE_CONTENTS_STRING });

				rootNode.AddChildNode(std::move(fileContentsNode));
			}

			return rootNode;
		}
	}
}