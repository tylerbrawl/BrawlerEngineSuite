module;
#include <string>

module Brawler.PSODefinitionsBaseFileWriter;
import Brawler.FileStrings;

/*
Source File Name: PSODefinitionBase.ixx

Contents:

// WARNING: This file was auto-generated by the Brawler Shader Compiler. You will incur the
// wrath of God if you dare touch it.

module;

export module Brawler.PSOs.PSODefinition:PSODefinitionBase;
import Brawler.PSOs.PSOID;

export namespace Brawler
{
	namespace PSOs
	{
		template <PSOID PSOIdentifier>
		struct PSODefinition
		{};
	}
}
*/

namespace Brawler
{
	namespace SourceFileWriters
	{
		PSODefinitionsBaseFileWriter::PSODefinitionsBaseFileWriter() :
			I_SourceFileWriter(L"PSODefinitionBase.ixx")
		{}

		Brawler::FileWriterNode PSODefinitionsBaseFileWriter::CreateFileWriterTree() const
		{
			Brawler::FileWriterNode rootNode{};

			{
				Brawler::FileWriterNode headerNode{};
				
				std::string headerText{ Brawler::FileStrings::AUTO_GENERATED_WARNING_COMMENT };
				headerText += "module;\n\nexport module Brawler.PSOs.PSODefinition:PSODefinitionBase;\nimport Brawler.PSOs.PSOID;\n\n";

				headerNode.SetOutputText(std::move(headerText));
				rootNode.AddChildNode(std::move(headerNode));
			}

			{
				Brawler::FileWriterNode namespaceNode{};

				{
					Brawler::FileWriterNode beginNamespaceNode{};
					beginNamespaceNode.SetOutputText("export namespace Brawler\n{\n\tnamespace PSOs\n\t{\n");

					namespaceNode.AddChildNode(std::move(beginNamespaceNode));
				}

				{
					Brawler::FileWriterNode baseTemplateNode{};
					baseTemplateNode.SetOutputText("\t\ttemplate <PSOID PSOIdentifier>\n\t\tstruct PSODefinition\n\t\t{};\n");

					namespaceNode.AddChildNode(std::move(baseTemplateNode));
				}

				{
					Brawler::FileWriterNode endNamespaceNode{};
					endNamespaceNode.SetOutputText("\t}\n}");

					namespaceNode.AddChildNode(std::move(endNamespaceNode));
				}

				rootNode.AddChildNode(std::move(namespaceNode));
			}

			return rootNode;
		}
	}
}