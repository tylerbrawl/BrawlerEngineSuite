module;
#include <string>
#include <array>
#include <span>

export module Brawler.RootParameterEnumsFileWriter;
import Brawler.I_SourceFileWriter;
import Brawler.ShaderProfileID;
import Brawler.FileStrings;
import Brawler.RootSignatureID;
import Brawler.RootParameters;
import Brawler.RootParameterDefinition;
import Brawler.RootSignatureDefinition;
import Brawler.JobSystem;
import Brawler.ShaderProfileDefinition;

/*
Source File Name: RootParameterEnums.ixx

Contents:

// WARNING: This file was auto-generated by the Brawler Shader Compiler. You will incur the
// wrath of God if you dare touch it.

module;

export module Brawler.RootParameters.RootParameterEnums;

export namespace Brawler
{
	namespace RootParameters
	{
		enum class RootParameterType
		{
			CBV,
			SRV,
			UAV,
			ROOT_CONSTANT,
			DESCRIPTOR_TABLE
		};

		enum class RootSignatureX
		{
			PARAM_NAME_0,
			PARAM_NAME_1,

			\\ This continues for all of the parameter names for this root signature's root parameters.

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
		class RootParameterEnumsFileWriter final : public I_SourceFileWriter
		{
		public:
			RootParameterEnumsFileWriter();

			RootParameterEnumsFileWriter(const RootParameterEnumsFileWriter& rhs) = delete;
			RootParameterEnumsFileWriter& operator=(const RootParameterEnumsFileWriter& rhs) = delete;

			RootParameterEnumsFileWriter(RootParameterEnumsFileWriter&& rhs) noexcept = default;
			RootParameterEnumsFileWriter& operator=(RootParameterEnumsFileWriter&& rhs) noexcept = default;

		protected:
			Brawler::FileWriterNode CreateFileWriterTree() const override;
		};
	}
}

// ---------------------------------------------------------------------------------------------------------------

namespace
{
	template <Brawler::RootSignatureID RSIdentifier>
	Brawler::FileWriterNode CreateRootParameterEnumNode()
	{
		using RootParamEnumType = Brawler::RootParamType<RSIdentifier>;
		
		std::string rootParamEnumStr{ "\n\t\tenum class " };
		rootParamEnumStr += Brawler::RootParameters::GetEnumClassNameString<RootParamEnumType>();
		rootParamEnumStr += "\n\t\t{\n";

		auto strSpan{ Brawler::RootParameters::GetEnumValueStrings<RootParamEnumType>() };
		for (const auto& rootParamStr : strSpan)
			rootParamEnumStr += "\t\t\t" + std::string{ rootParamStr } + ",\n";

		rootParamEnumStr += "\n\t\t\tCOUNT_OR_ERROR\n\t\t};\n";

		Brawler::FileWriterNode rootParamEnumNode{};
		rootParamEnumNode.SetOutputText(std::move(rootParamEnumStr));

		return rootParamEnumNode;
			
	}

	template <Brawler::RootSignatureID RSIdentifier>
	void AddNodeCreationJob(Brawler::FileWriterNode& node, Brawler::JobGroup& jobGroup)
	{
		jobGroup.AddJob([&node] ()
		{
			node = CreateRootParameterEnumNode<RSIdentifier>();
		});
	}

	template <std::underlying_type_t<Brawler::RootSignatureID>... RSIdentifierNums>
	Brawler::FileWriterNode CreateRootParameterEnumListNode(std::integer_sequence<std::underlying_type_t<Brawler::RootSignatureID>, RSIdentifierNums...> rsSequence)
	{
		Brawler::FileWriterNode rootParamEnumListNode{};
		std::array<Brawler::FileWriterNode, rsSequence.size()> childNodeArr{};

		Brawler::JobGroup createChildNodesJobGroup{};
		createChildNodesJobGroup.Reserve(rsSequence.size());

		std::size_t currIndex = 0;
		(AddNodeCreationJob<static_cast<Brawler::RootSignatureID>(RSIdentifierNums)>(childNodeArr[currIndex++], createChildNodesJobGroup), ...);

		createChildNodesJobGroup.ExecuteJobs();

		for (auto&& childNode : childNodeArr)
			rootParamEnumListNode.AddChildNode(std::move(childNode));

		return rootParamEnumListNode;
	}
}

namespace Brawler
{
	namespace SourceFileWriters
	{
		template <Brawler::ShaderProfiles::ShaderProfileID ProfileID>
		RootParameterEnumsFileWriter<ProfileID>::RootParameterEnumsFileWriter() :
			I_SourceFileWriter(L"RootParameterEnums.ixx")
		{}

		template <Brawler::ShaderProfiles::ShaderProfileID ProfileID>
		Brawler::FileWriterNode RootParameterEnumsFileWriter<ProfileID>::CreateFileWriterTree() const
		{
			Brawler::FileWriterNode rootNode{};

			{
				std::string preParamEnumListStr{ Brawler::FileStrings::AUTO_GENERATED_WARNING_COMMENT };
				preParamEnumListStr += "module;\n\nexport module Brawler.RootParameters.RootParameterEnums;\n\nexport namespace Brawler\n{\n\tnamespace RootParameters\n\t{\n\t\tenum class RootParameterType\n\t\t{\n\t\t\tCBV,\n\t\t\tSRV,\n\t\t\tUAV,\n\t\t\tROOT_CONSTANT,\n\t\t\tDESCRIPTOR_TABLE\n\t\t};\n";

				Brawler::FileWriterNode preParamEnumListNode{};
				preParamEnumListNode.SetOutputText(std::move(preParamEnumListStr));

				rootNode.AddChildNode(std::move(preParamEnumListNode));
			}

			rootNode.AddChildNode(CreateRootParameterEnumListNode(Brawler::ShaderProfiles::GetRootSignatureIdentifiers<ProfileID>()));

			{
				Brawler::FileWriterNode postParamEnumListNode{};
				postParamEnumListNode.SetOutputText("\t}\n}");

				rootNode.AddChildNode(std::move(postParamEnumListNode));
			}

			return rootNode;
		}
	}
}