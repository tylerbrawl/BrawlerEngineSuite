module;
#include <string>

export module Brawler.PSOIDsFileWriter;
import Brawler.I_SourceFileWriter;
import Brawler.ShaderProfileID;
import Brawler.ShaderProfileDefinition;
import Brawler.FileWriterNode;
import Brawler.FileStrings;
import Brawler.PSOID;
import Brawler.PSODefinition;

/*
Source File Name: PSOID.ixx

Contents:

// WARNING: This file was auto-generated by the Brawler Shader Compiler. You will incur the
// wrath of God if you dare touch it.

module;

export module Brawler.PSOs.PSOID;

export namespace Brawler
{
	namespace PSOs
	{
		enum class PSOID
		{
			X,
			Y,

			\\ This continues for all of the pipeline state objects relevant to the current shader profile.

			COUNT_OR_ERROR
		};
	}
}
*/

export namespace Brawler
{
	namespace SourceFileWriters
	{
		template <Brawler::ShaderProfiles::ShaderProfileID ProfileID>
		class PSOIDsFileWriter final : public I_SourceFileWriter
		{
		public:
			PSOIDsFileWriter();

			PSOIDsFileWriter(const PSOIDsFileWriter& rhs) = delete;
			PSOIDsFileWriter& operator=(const PSOIDsFileWriter& rhs) = delete;

			PSOIDsFileWriter(PSOIDsFileWriter&& rhs) noexcept = default;
			PSOIDsFileWriter& operator=(PSOIDsFileWriter&& rhs) noexcept = default;

		protected:
			Brawler::FileWriterNode CreateFileWriterTree() const override;
		};
	}
}

// --------------------------------------------------------------------------------------------------------

namespace
{
	template <std::underlying_type_t<Brawler::PSOID>... PSOIdentifierNums>
	Brawler::FileWriterNode CreatePSOIdentifierEnumListNode(std::integer_sequence<std::underlying_type_t<Brawler::PSOID>, PSOIdentifierNums...>)
	{
		std::string enumListStr{};
		((enumListStr += "\t\t\t" + std::string{ Brawler::GetPSOIDString<static_cast<Brawler::PSOID>(PSOIdentifierNums)>() } + ",\n"), ...);

		enumListStr += "\n\t\t\tCOUNT_OR_ERROR\n";

		Brawler::FileWriterNode enumListNode{};
		enumListNode.SetOutputText(std::move(enumListStr));

		return enumListNode;
	}
}

namespace Brawler
{
	namespace SourceFileWriters
	{
		template <Brawler::ShaderProfiles::ShaderProfileID ProfileID>
		PSOIDsFileWriter<ProfileID>::PSOIDsFileWriter() :
			I_SourceFileWriter(L"PSOID.ixx")
		{}

		template <Brawler::ShaderProfiles::ShaderProfileID ProfileID>
		Brawler::FileWriterNode PSOIDsFileWriter<ProfileID>::CreateFileWriterTree() const
		{
			Brawler::FileWriterNode rootNode{};

			{
				Brawler::FileWriterNode headerNode{};

				std::string headerText{ Brawler::FileStrings::AUTO_GENERATED_WARNING_COMMENT };
				headerText += "module;\n\nexport module Brawler.PSOs.PSOID;\n\n";

				headerNode.SetOutputText(std::move(headerText));
				rootNode.AddChildNode(std::move(headerNode));
			}

			{
				Brawler::FileWriterNode beginEnumNode{};
				beginEnumNode.SetOutputText("export namespace Brawler\n{\n\tnamespace PSOs\n\t{\n\t\tenum class PSOID\n\t\t{\n");

				rootNode.AddChildNode(std::move(beginEnumNode));
			}

			rootNode.AddChildNode(CreatePSOIdentifierEnumListNode(Brawler::ShaderProfiles::GetPSOIdentifiers<ProfileID>()));

			{
				Brawler::FileWriterNode endEnumNode{};
				endEnumNode.SetOutputText("\t\t};\n\t}\n}");

				rootNode.AddChildNode(std::move(endEnumNode));
			}

			return rootNode;
		}
	}
}