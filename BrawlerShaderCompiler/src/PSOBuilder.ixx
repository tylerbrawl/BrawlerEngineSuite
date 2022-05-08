module;
#include <vector>
#include <memory>
#include <string>
#include <span>
#include "DxDef.h"

export module Brawler.PSOBuilder;
import Brawler.PSOID;
import Brawler.PSODefinition;
import Brawler.RootSignatureDefinition;
import Brawler.RootSignatureID;
import Brawler.PSOField;
import Brawler.FileWriterNode;
import Brawler.I_PSOFieldResolver;
import Util.Reflection;
import Util.FileWrite;

export namespace Brawler
{
	namespace PSOs
	{
		template <Brawler::PSOID PSOIdentifier>
		class PSOBuilder
		{
		public:
			using PSOStreamType = Brawler::PSOStreamType<PSOIdentifier>;

		public:
			PSOBuilder() = default;

			PSOBuilder(const PSOBuilder& rhs) = default;
			PSOBuilder& operator=(const PSOBuilder& rhs) = default;

			PSOBuilder(PSOBuilder&& rhs) noexcept = default;
			PSOBuilder& operator=(PSOBuilder&& rhs) noexcept = default;

			void SetPSODefaultValue(PSOStreamType&& psoDesc);

			template <typename T>
				requires std::derived_from<T, Brawler::I_PSOFieldResolver>
			void AddPSOFieldResolver(T&& fieldResolver);

			Brawler::FileWriterNode CreatePSODefinitionFileWriterNode() const;

		private:
			PSOStreamType mPSODefaultValue;
			std::vector<std::unique_ptr<Brawler::I_PSOFieldResolver>> mFieldResolverArr;
		};
	}
}

// ------------------------------------------------------------------------------------------------------

namespace
{
	template <typename PSOStreamType, std::size_t FieldIndex>
	void AddPSOStreamFieldToString(std::string& str)
	{
		using CurrFieldType = Util::Reflection::FieldType<PSOStreamType, FieldIndex>;

		// Add the type of the field to the string, followed by the field's name.
		str += "\n\t\t\t\t";
		str += Brawler::PSOField<CurrFieldType>::FIELD_TYPE;
		str += " ";
		str += Brawler::PSOField<CurrFieldType>::FIELD_NAME;
		str += ";";

		// Terminate the recursive loop if we have added all of the fields to the string.
		if constexpr ((FieldIndex + 1) != Util::Reflection::GetFieldCount<PSOStreamType>())
			AddPSOStreamFieldToString<PSOStreamType, (FieldIndex + 1)>(str);
	}



	template <Brawler::PSOID PSOIdentifier>
	Brawler::FileWriterNode CreatePSOStreamTypeDefinitionNode()
	{
		// If things haven't seemed weird yet, this is where they get *REALLY* weird. Basically,
		// we're going to be using static reflection in order to infer the contents of the
		// PSOID's corresponding PSOStreamType. That way, we can write them out to the source
		// file.

		// Don't add the trailing new line character; this is added by AddPSOStreamFieldToString().
		std::string psoStreamDefinitionStr{ "\t\t\tstruct PSOStreamType\n\t\t\t{" };

		AddPSOStreamFieldToString<Brawler::PSOStreamType<PSOIdentifier>, 0>(psoStreamDefinitionStr);

		psoStreamDefinitionStr += "\n\t\t\t};\n\n";

		Brawler::FileWriterNode psoStreamDefinitionNode{};
		psoStreamDefinitionNode.SetOutputText(std::move(psoStreamDefinitionStr));

		return psoStreamDefinitionNode;
	}
}

namespace Brawler
{
	namespace PSOs
	{
		template <Brawler::PSOID PSOIdentifier>
		void PSOBuilder<PSOIdentifier>::SetPSODefaultValue(PSOStreamType&& psoDesc)
		{
			mPSODefaultValue = std::move(psoDesc);
		}

		template <Brawler::PSOID PSOIdentifier>
		template <typename T>
			requires std::derived_from<T, Brawler::I_PSOFieldResolver>
		void PSOBuilder<PSOIdentifier>::AddPSOFieldResolver(T&& fieldResolver)
		{
			mFieldResolverArr.push_back(std::make_unique<T>(std::forward<T>(fieldResolver)));
		}

		template <Brawler::PSOID PSOIdentifier>
		Brawler::FileWriterNode PSOBuilder<PSOIdentifier>::CreatePSODefinitionFileWriterNode() const
		{
			Brawler::FileWriterNode psoDefinitionRootNode{};

			{
				std::string psoDefinitionStr{ "\t\ttemplate <>\n\t\tstruct PSODefinition<PSOID::" };
				psoDefinitionStr += Brawler::GetPSOIDString<PSOIdentifier>();
				psoDefinitionStr += ">\n\t\t{\n"; 
				
				Brawler::FileWriterNode beginDefinitionNode{};
				beginDefinitionNode.SetOutputText(std::move(psoDefinitionStr));
				psoDefinitionRootNode.AddChildNode(std::move(beginDefinitionNode));
			}

			psoDefinitionRootNode.AddChildNode(CreatePSOStreamTypeDefinitionNode<PSOIdentifier>());

			// Write out the default PSO value as a byte array.
			{
				std::string defaultPSOValueStr{ "\t\t\tstatic constexpr std::array<std::uint8_t, sizeof(PSOStreamType)> DEFAULT_PSO_VALUE{" };
				defaultPSOValueStr += Util::FileWrite::CreateSTDUInt8ArrayContentsStringFromBuffer(std::span<const std::uint8_t, sizeof(PSOStreamType)>{ reinterpret_cast<const std::uint8_t*>(&mPSODefaultValue), sizeof(PSOStreamType) });
				defaultPSOValueStr += "};\n";

				Brawler::FileWriterNode defaultPSOValueNode{};
				defaultPSOValueNode.SetOutputText(std::move(defaultPSOValueStr));
				psoDefinitionRootNode.AddChildNode(std::move(defaultPSOValueNode));
			}

			{
				std::string rootSignatureIDStr{ "\t\t\tstatic constexpr Brawler::RootSignatures::RootSignatureID ROOT_SIGNATURE_ID = Brawler::RootSignatures::RootSignatureID::" };
				rootSignatureIDStr += Brawler::GetRootSignatureIDString<Brawler::GetRootSignatureID<PSOIdentifier>()>();
				rootSignatureIDStr += ";\n\n";

				Brawler::FileWriterNode rootSignatureIDNode{};
				rootSignatureIDNode.SetOutputText(std::move(rootSignatureIDStr));
				psoDefinitionRootNode.AddChildNode(std::move(rootSignatureIDNode));
			}

			// Each of this PSOBuilder's I_PSOFieldResolver instances may have an additional field which they wish
			// to add to this structure.
			for (const auto& fieldResolverPtr : mFieldResolverArr)
				psoDefinitionRootNode.AddChildNode(fieldResolverPtr->CreatePSODefinitionFileWriterNode());

			{
				Brawler::FileWriterNode runtimeResolutionNode{};

				{
					Brawler::FileWriterNode beginRuntimeResolutionNode{};
					beginRuntimeResolutionNode.SetOutputText("\n\t\t\tstatic void ExecuteRuntimePSOResolution(PSOStreamType& psoDesc)\n\t\t\t{\n");

					runtimeResolutionNode.AddChildNode(std::move(beginRuntimeResolutionNode));
				}

				// Likewise, these I_PSOFieldResolver instances may have additional instructions which need to
				// be executed in order to "fix" the PSO description for use with the D3D12 API.
				for (const auto& fieldResolverPtr : mFieldResolverArr)
					runtimeResolutionNode.AddChildNode(fieldResolverPtr->CreateRuntimePSOResolutionFileWriterNode());

				// In addition to these, every PSO has a root signature, so we can add its resolution here.
				{
					std::string rsResolveStr{ "\t\t\t\tpsoDesc." };
					rsResolveStr += Brawler::PSOField<CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE>::FIELD_NAME;
					rsResolveStr += " = &(Brawler::D3D12::RootSignatureDatabase::GetInstance().GetRootSignature<ROOT_SIGNATURE_ID>());\n";

					Brawler::FileWriterNode rootSignatureResolveNode{};
					rootSignatureResolveNode.SetOutputText(std::move(rsResolveStr));

					runtimeResolutionNode.AddChildNode(std::move(rootSignatureResolveNode));
				}

				{
					Brawler::FileWriterNode endRuntimeResolutionNode{};
					endRuntimeResolutionNode.SetOutputText("\t\t\t}\n");

					runtimeResolutionNode.AddChildNode(std::move(endRuntimeResolutionNode));
				}

				psoDefinitionRootNode.AddChildNode(std::move(runtimeResolutionNode));
			}

			{
				Brawler::FileWriterNode endPSODefinitionNode{};
				endPSODefinitionNode.SetOutputText("\n\t\t};\n");

				psoDefinitionRootNode.AddChildNode(std::move(endPSODefinitionNode));
			}

			return psoDefinitionRootNode;
		}
	}
}