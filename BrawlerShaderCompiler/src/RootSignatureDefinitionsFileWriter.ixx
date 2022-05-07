module;
#include <span>
#include <vector>
#include <string>
#include <memory>

export module Brawler.RootSignatureDefinitionsFileWriter;
import Brawler.I_SourceFileWriter;
import Brawler.ConvenienceTypes;
import Brawler.ShaderProfileID;
import Brawler.RootSignatureID;
import Brawler.ShaderProfileDefinition;
import Brawler.RootSignatureBuilderCreators;
import Brawler.RootSignatureBuilder;
import Brawler.FileWriterNode;
import Brawler.FileStrings;
import Brawler.JobSystem;

/*
Source File Name: RootSignatureDefinition.ixx

Contents:

// WARNING: This file was auto-generated by the Brawler Shader Compiler. You will incur the
// wrath of God if you dare touch it.

module;
#include <array>
#include <span>

export module Brawler.RootSignatures.RootSignatureDefinition;
import Brawler.RootSignatures.RootSignatureID;
import Brawler.RootParameters.RootParameterEnums;

namespace Brawler
{
	namespace RootSignatures
	{
		template <RootSignatureID RSIdentifier>
		struct RootSignatureDefinition
		{};

		template <>
		struct RootSignatureDefinition<RootSignatureID::X>
		{
			static constexpr std::array<std::uint8_t, X> SERIALIZED_ROOT_SIGNATURE_VERSION_1_0{...};
			static constexpr std::array<std::uint8_t, X> SERIALIZED_ROOT_SIGNATURE_VERSION_1_1{...};

			using RootParamEnumType = X;

			static constexpr std::array<Brawler::RootParameters::RootParameterType, std::to_underlying(RootParamEnumType::COUNT_OR_ERROR)> ROOT_PARAM_TYPES_ARR{...};
		};

		\\ This continues on for the rest of the required root signatures.
	}
}

export namespace Brawler
{
	namespace RootSignatures
	{
		template <RootSignatureID RSIdentifier>
		using RootParamEnumType = RootSignatureDefinition<RSIdentifier>::RootParamEnumType;

		template <RootSignatureID RSIdentifier>
		consteval auto GetSerializedRootSignature1_0()
		{
			return std::span<const std::uint8_t, RootSignatureDefinition<RSIdentifier>::SERIALIZED_ROOT_SIGNATURE_VERSION_1_0.size()>{ RootSignatureDefinition<RSIdentifier>::SERIALIZED_ROOT_SIGNATURE_VERSION_1_0 };
		}

		template <RootSignatureID RSIdentifier>
		consteval auto GetSerializedRootSignature1_1()
		{
			return std::span<const std::uint8_t, RootSignatureDefinition<RSIdentifier>::SERIALIZED_ROOT_SIGNATURE_VERSION_1_1.size()>{ RootSignatureDefinition<RSIdentifier>::SERIALIZED_ROOT_SIGNATURE_VERSION_1_1 };
		}

		template <RootSignatureID RSIdentifier, RootParamEnumType<RSIdentifier> RootParam>
		consteval Brawler::RootParameters::RootParameterType GetRootParameterType()
		{
			return RootSignatureDefinition<RSIdentifier>::ROOT_PARAM_TYPES_ARR[std::to_underlying(RootParam)];
		}

		template <RootSignatureID RSIdentifier>
		consteval std::size_t GetRootParameterCount()
		{
			return static_cast<std::size_t>(RootParamEnumType<RSIdentifier>::COUNT_OR_ERROR);
		}
	}
}
*/

export namespace Brawler
{
	namespace SourceFileWriters
	{
		template <Brawler::ShaderProfiles::ShaderProfileID ProfileID>
		class RootSignatureDefinitionsFileWriter final : public I_SourceFileWriter
		{
		public:
			RootSignatureDefinitionsFileWriter();

			RootSignatureDefinitionsFileWriter(const RootSignatureDefinitionsFileWriter& rhs) = delete;
			RootSignatureDefinitionsFileWriter& operator=(const RootSignatureDefinitionsFileWriter& rhs) = delete;

			RootSignatureDefinitionsFileWriter(RootSignatureDefinitionsFileWriter&& rhs) noexcept = default;
			RootSignatureDefinitionsFileWriter& operator=(RootSignatureDefinitionsFileWriter&& rhs) noexcept = default;

		protected:
			Brawler::FileWriterNode CreateFileWriterTree() const override;
		};
	}
}

// ---------------------------------------------------------------------------------------------------------------------------

namespace
{
	// For some reason, we cannot define this within a lambda function. Furthermore, template pack expansion
	// to produce multiple templated lambda function calls also does not work. To overcome this, we create
	// this function outside of CreateRootSignatureDefinitionNodes().
	//
	// I'm not sure if this is a shortcoming of either the MSVC or the C++ Standard.
	template <Brawler::RootSignatureID CurrRSIdentifier>
	static void AddNodeCreationJob(Brawler::JobGroup& jobGroup, std::shared_ptr<std::vector<Brawler::FileWriterNode>>& nodeArr, std::size_t& nodeIndex)
	{
		const std::size_t currIndex = nodeIndex++;
		jobGroup.AddJob([nodeArr, currIndex] ()
			{
				Brawler::FileWriterNode& currNode{ (*nodeArr)[currIndex] };
				Brawler::RootSignatures::RootSignatureBuilder<CurrRSIdentifier> rootSigBuilder{ Brawler::RootSignatures::CreateRootSignatureBuilder<CurrRSIdentifier>() };
				currNode = rootSigBuilder.CreateRootSignatureDefinitionFileWriterNode();
			});
	};

	template <Brawler::ShaderProfiles::ShaderProfileID ProfileID>
	std::vector<Brawler::FileWriterNode> CreateRootSignatureDefinitionNodes()
	{
		static const auto createJobsLambda = [] <std::underlying_type_t<Brawler::RootSignatureID>... RSIdentifierNums> (std::integer_sequence<std::underlying_type_t<Brawler::RootSignatureID>, RSIdentifierNums...> rsSequence) -> std::vector<Brawler::FileWriterNode>
		{
			Brawler::JobGroup templateInstantiationNodeJobGroup{};
			templateInstantiationNodeJobGroup.Reserve(rsSequence.size());

			std::shared_ptr<std::vector<Brawler::FileWriterNode>> templateInstantiationNodeArr{ std::make_shared<std::vector<Brawler::FileWriterNode>>() };
			templateInstantiationNodeArr->resize(rsSequence.size());

			std::size_t currNodeArrIndex = 0;

			((AddNodeCreationJob<static_cast<Brawler::RootSignatureID>(RSIdentifierNums)>(templateInstantiationNodeJobGroup, templateInstantiationNodeArr, currNodeArrIndex)), ...);

			templateInstantiationNodeJobGroup.ExecuteJobs();

			return std::move(*templateInstantiationNodeArr);
		};

		return createJobsLambda(Brawler::ShaderProfiles::GetRootSignatureIdentifiers<ProfileID>());
	}
}

namespace Brawler
{
	namespace SourceFileWriters
	{
		template <Brawler::ShaderProfiles::ShaderProfileID ProfileID>
		RootSignatureDefinitionsFileWriter<ProfileID>::RootSignatureDefinitionsFileWriter() :
			I_SourceFileWriter(L"RootSignatureDefinition.ixx")
		{}

		template <Brawler::ShaderProfiles::ShaderProfileID ProfileID>
		Brawler::FileWriterNode RootSignatureDefinitionsFileWriter<ProfileID>::CreateFileWriterTree() const
		{
			Brawler::FileWriterNode rootNode{};

			{
				Brawler::FileWriterNode headerNode{};

				std::string headerNodeStr{ Brawler::FileStrings::AUTO_GENERATED_WARNING_COMMENT };
				headerNodeStr += "module;\n#include <array>\n#include <span>\n\nexport module Brawler.RootSignatures.RootSignatureDefinition;\nimport Brawler.RootSignatures.RootSignatureID;\nimport Brawler.RootParameters.RootParameterEnums;\n\n";

				headerNode.SetOutputText(std::move(headerNodeStr));

				rootNode.AddChildNode(std::move(headerNode));
			}

			{
				Brawler::FileWriterNode internalNamespaceNode{};

				{
					Brawler::FileWriterNode beginInternalNamespaceNode{};
					beginInternalNamespaceNode.SetOutputText("namespace Brawler\n{\n\tnamespace RootSignatures\n\t{\n");

					internalNamespaceNode.AddChildNode(std::move(beginInternalNamespaceNode));
				}

				{
					Brawler::FileWriterNode defaultTemplateDefinitionNode{};
					defaultTemplateDefinitionNode.SetOutputText("\t\ttemplate <RootSignatureID RSIdentifier>\n\t\tstruct RootSignatureDefinition\n\t\t{};\n\n");

					internalNamespaceNode.AddChildNode(std::move(defaultTemplateDefinitionNode));
				}

				{
					std::vector<Brawler::FileWriterNode> rootSignatureDefinitionNodeArr{ CreateRootSignatureDefinitionNodes<ProfileID>() };

					for (auto&& node : rootSignatureDefinitionNodeArr)
						internalNamespaceNode.AddChildNode(std::move(node));
				}

				{
					Brawler::FileWriterNode endInternalNamespaceNode{};
					endInternalNamespaceNode.SetOutputText("\t}\n}\n\n");

					internalNamespaceNode.AddChildNode(std::move(endInternalNamespaceNode));
				}

				rootNode.AddChildNode(std::move(internalNamespaceNode));
			}

			{
				Brawler::FileWriterNode exportedNamespaceNode{};

				{
					Brawler::FileWriterNode beginExportedNamespaceNode{};
					beginExportedNamespaceNode.SetOutputText("export namespace Brawler\n{\n\tnamespace RootSignatures\n\t{\n");

					exportedNamespaceNode.AddChildNode(std::move(beginExportedNamespaceNode));
				}

				{
					Brawler::FileWriterNode rootParamEnumTypeNode{};
					rootParamEnumTypeNode.SetOutputText("\t\ttemplate <RootSignatureID RSIdentifier>\n\t\tusing RootParamEnumType = RootSignatureDefinition<RSIdentifier>::RootParamEnumType;\n\n");

					exportedNamespaceNode.AddChildNode(std::move(rootParamEnumTypeNode));
				}

				{
					Brawler::FileWriterNode getSerializedRootSignature1_0Node{};
					getSerializedRootSignature1_0Node.SetOutputText("\t\ttemplate <RootSignatureID RSIdentifier>\n\t\tconsteval auto GetSerializedRootSignature1_0()\n\t\t{\n\t\t\treturn std::span<const std::uint8_t, RootSignatureDefinition<RSIdentifier>::SERIALIZED_ROOT_SIGNATURE_VERSION_1_0.size()>{ RootSignatureDefinition<RSIdentifier>::SERIALIZED_ROOT_SIGNATURE_VERSION_1_0 };\n\t\t}\n\n");

					exportedNamespaceNode.AddChildNode(std::move(getSerializedRootSignature1_0Node));
				}

				{
					Brawler::FileWriterNode getSerializedRootSignature1_1Node{};
					getSerializedRootSignature1_1Node.SetOutputText("\t\ttemplate <RootSignatureID RSIdentifier>\n\t\tconsteval auto GetSerializedRootSignature1_1()\n\t\t{\n\t\t\treturn std::span<const std::uint8_t, RootSignatureDefinition<RSIdentifier>::SERIALIZED_ROOT_SIGNATURE_VERSION_1_1.size()>{ RootSignatureDefinition<RSIdentifier>::SERIALIZED_ROOT_SIGNATURE_VERSION_1_1 };\n\t\t}\n\n");

					exportedNamespaceNode.AddChildNode(std::move(getSerializedRootSignature1_1Node));
				}

				{
					Brawler::FileWriterNode getRootParameterTypeNode{};
					getRootParameterTypeNode.SetOutputText("\t\ttemplate <RootSignatureID RSIdentifier, RootParamEnumType<RSIdentifier> RootParam>\n\t\tconsteval Brawler::RootParameters::RootParameterType GetRootParameterType()\n\t\t{\n\t\t\treturn RootSignatureDefinition<RSIdentifier>::ROOT_PARAM_TYPES_ARR[std::to_underlying(RootParam)];\n\t\t}\n\n");

					exportedNamespaceNode.AddChildNode(std::move(getRootParameterTypeNode));
				}

				{
					Brawler::FileWriterNode getRootParameterCountNode{};
					getRootParameterCountNode.SetOutputText("\t\ttemplate <RootSignatureID RSIdentifier>\n\t\tconsteval std::size_t GetRootParameterCount()\n\t\t{\n\t\t\treturn static_cast<std::size_t>(RootParamEnumType<RSIdentifier>::COUNT_OR_ERROR);\n\t\t}\n");

					exportedNamespaceNode.AddChildNode(std::move(getRootParameterCountNode));
				}

				{
					Brawler::FileWriterNode endExportedNamespaceNode{};
					endExportedNamespaceNode.SetOutputText("\t}\n}");

					exportedNamespaceNode.AddChildNode(std::move(endExportedNamespaceNode));
				}

				rootNode.AddChildNode(std::move(exportedNamespaceNode));
			}

			return rootNode;
		}
	}
}