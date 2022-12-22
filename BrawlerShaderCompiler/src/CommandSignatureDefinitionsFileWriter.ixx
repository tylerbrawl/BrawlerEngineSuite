module;
#include <string>
#include <ranges>
#include <string_view>
#include <format>

export module Brawler.CommandSignatureDefinitionsFileWriter;
import Brawler.I_SourceFileWriter;
import Brawler.FileWriterNode;
import Brawler.ShaderProfileID;
import Brawler.ShaderProfileDefinition;
import Brawler.FileStrings;
import Brawler.CommandSignatureDefinition;
import Brawler.CommandSignatureID;

/*
Source File Name: CommandSignatureDefinition.ixx

Contents:

// WARNING: This file was auto-generated by the Brawler Shader Compiler. You will incur the
// wrath of God if you dare touch it.

module;
#include <optional>
#include "../DxDef.h"

export module Brawler.CommandSignatures.CommandSignatureDefinition;
import :CommandSignatureDefinitionBase;

\\ We then export import every module partition unit which represents a specialization of
\\ CommandSignatureDefinition.

import Brawler.CommandSignatures.CommandSignatureID;
import Brawler.RootSignatures.RootSignatureID;

export namespace Brawler
{
	namespace CommandSignatures
	{
		template <CommandSignatureID CSIdentifier>
		using CommandSignatureType = typename CommandSignatureDefinition<CSIdentifier>::CommandSignatureType;

		template <CommandSignatureID CSIdentifier>
		consteval D3D12_COMMAND_SIGNATURE_DESC CreateCommandSignatureDescription()
		{
			return CommandSignatureDefinition<CSIdentifier>::COMMAND_SIGNATURE_DESCRIPTION;
		}

		template <CommandSignatureID CSIdentifier>
		consteval std::optional<RootSignatures::RootSignatureID> GetRootSignatureForCommandSignature()
		{
			constexpr RootSignatures::RootSignatureID ASSOCIATED_ROOT_SIGNATURE_ID = CommandSignatureDefinition<CSIdentifier>::ASSOCIATED_ROOT_SIGNATURE_ID;

			if constexpr (ASSOCIATED_ROOT_SIGNATURE_ID == RootSignatures::RootSignatureID::COUNT_OR_ERROR)
				return {};
			else
				return ASSOCIATED_ROOT_SIGNATURE_ID;
		}
	}
}
*/

export namespace Brawler
{
	namespace SourceFileWriters
	{
		template <ShaderProfiles::ShaderProfileID ProfileID>
		class CommandSignatureDefinitionsFileWriter final : public I_SourceFileWriter
		{
		public:
			CommandSignatureDefinitionsFileWriter();

			CommandSignatureDefinitionsFileWriter(const CommandSignatureDefinitionsFileWriter& rhs) = delete;
			CommandSignatureDefinitionsFileWriter& operator=(const CommandSignatureDefinitionsFileWriter& rhs) = delete;

			CommandSignatureDefinitionsFileWriter(CommandSignatureDefinitionsFileWriter&& rhs) noexcept = default;
			CommandSignatureDefinitionsFileWriter& operator=(CommandSignatureDefinitionsFileWriter&& rhs) noexcept = default;

		protected:
			Brawler::FileWriterNode CreateFileWriterTree() const override;
		};
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace SourceFileWriters
	{
		template <ShaderProfiles::ShaderProfileID ProfileID>
		constexpr std::string CreateExportImportString()
		{
			constexpr auto RELEVANT_COMMAND_SIGNATURE_IDS_ARR{ ShaderProfiles::GetCommandSignatureIdentifiers<ProfileID>() };
			constexpr auto ADD_EXPORT_IMPORT_STR_LAMBDA = []<std::size_t CurrIndex>(this const auto& self, std::string& exportImportStr)
			{
				if constexpr (CurrIndex != RELEVANT_COMMAND_SIGNATURE_IDS_ARR.size())
				{
					constexpr CommandSignatureID CURR_IDENTIFIER = RELEVANT_COMMAND_SIGNATURE_IDS_ARR[CurrIndex];
					constexpr std::string_view CURR_ID_STR{ Brawler::GetCommandSignatureIDString<CURR_IDENTIFIER>() };

					exportImportStr += "export import :CommandSignatureDefinition_";
					exportImportStr += CURR_ID_STR;
					exportImportStr += ";\n";

					constexpr std::size_t NEXT_INDEX = (CurrIndex + 1);
					self.template operator()<NEXT_INDEX>(exportImportStr);
				}
			};

			std::string exportImportStr{};
			ADD_EXPORT_IMPORT_STR_LAMBDA.template operator()<0>(exportImportStr);

			return exportImportStr;
		}

		template <ShaderProfiles::ShaderProfileID ProfileID>
		static constexpr auto EXPORT_IMPORT_CHARACTER_ARR{ []()
		{
			constexpr std::size_t EXPORT_IMPORT_STR_SIZE = CreateExportImportString<ProfileID>().size();

			// Add one for the NUL-terminating character.
			std::array<char, (EXPORT_IMPORT_STR_SIZE + 1)> exportImportCharArr{};
			const std::string exportImportStr{ CreateExportImportString<ProfileID>() };

			for (auto [destChar, srcChar] : std::views::zip(exportImportCharArr, exportImportStr))
				destChar = srcChar;

			return exportImportCharArr;
		}() };

		template <ShaderProfiles::ShaderProfileID ProfileID>
		static constexpr std::string_view EXPORT_IMPORT_STR{ EXPORT_IMPORT_CHARACTER_ARR<ProfileID>.data() };
	}
}

namespace Brawler
{
	namespace SourceFileWriters
	{
		template <ShaderProfiles::ShaderProfileID ProfileID>
		CommandSignatureDefinitionsFileWriter<ProfileID>::CommandSignatureDefinitionsFileWriter() :
			I_SourceFileWriter(L"CommandSignatureDefinition.ixx")
		{}

		template <ShaderProfiles::ShaderProfileID ProfileID>
		Brawler::FileWriterNode CommandSignatureDefinitionsFileWriter<ProfileID>::CreateFileWriterTree() const
		{
			FileWriterNode rootNode{};

			{
				FileWriterNode autoGeneratedWarningNode{};
				autoGeneratedWarningNode.SetOutputText(std::string{ Brawler::FileStrings::AUTO_GENERATED_WARNING_COMMENT });

				rootNode.AddChildNode(std::move(autoGeneratedWarningNode));
			}

			{
				static constexpr std::string_view FILE_CONTENTS_FORMAT_STR{
R"(module;
#include <optional>
#include "../DxDef.h"

export module Brawler.CommandSignatures.CommandSignatureDefinition;
import :CommandSignatureDefinitionBase;
{}
import Brawler.CommandSignatures.CommandSignatureID;
import Brawler.RootSignatures.RootSignatureID;

export namespace Brawler
{{
	namespace CommandSignatures
	{{
		template <CommandSignatureID CSIdentifier>
		using CommandSignatureType = typename CommandSignatureDefinition<CSIdentifier>::CommandSignatureType;

		template <CommandSignatureID CSIdentifier>
		consteval D3D12_COMMAND_SIGNATURE_DESC CreateCommandSignatureDescription()
		{{
			return CommandSignatureDefinition<CSIdentifier>::COMMAND_SIGNATURE_DESCRIPTION;
		}}

		template <CommandSignatureID CSIdentifier>
		consteval std::optional<RootSignatures::RootSignatureID> GetRootSignatureForCommandSignature()
		{{
			constexpr RootSignatures::RootSignatureID ASSOCIATED_ROOT_SIGNATURE_ID = CommandSignatureDefinition<CSIdentifier>::ASSOCIATED_ROOT_SIGNATURE_ID;

			if constexpr (ASSOCIATED_ROOT_SIGNATURE_ID == RootSignatures::RootSignatureID::COUNT_OR_ERROR)
				return {{}};
			else
				return ASSOCIATED_ROOT_SIGNATURE_ID;
		}}
	}}
}})"
				};

				FileWriterNode fileContentsNode{};
				fileContentsNode.SetOutputText(std::format(FILE_CONTENTS_FORMAT_STR, EXPORT_IMPORT_STR<ProfileID>));

				rootNode.AddChildNode(std::move(fileContentsNode));
			}

			return rootNode;
		}
	}
}