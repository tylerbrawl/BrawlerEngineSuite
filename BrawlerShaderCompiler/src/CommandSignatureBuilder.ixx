module;
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <ranges>
#include <cassert>

export module Brawler.CommandSignatureBuilder;
import Brawler.CommandSignatureID;
import Brawler.IndirectArgumentResolvers;
import Brawler.FileWriterNode;
import Brawler.RootSignatureID;
import Brawler.RootSignatureDefinition;
import Brawler.CommandSignatureDefinition;

export namespace Brawler
{
	namespace CommandSignatures
	{
		template <CommandSignatureID CSIdentifier>
		class CommandSignatureBuilder
		{
		public:
			CommandSignatureBuilder() = default;

			CommandSignatureBuilder(const CommandSignatureBuilder& rhs) = delete;
			CommandSignatureBuilder& operator=(const CommandSignatureBuilder& rhs) = delete;

			CommandSignatureBuilder(CommandSignatureBuilder&& rhs) noexcept = default;
			CommandSignatureBuilder& operator=(CommandSignatureBuilder&& rhs) noexcept = default;

			template <typename T>
				requires std::derived_from<T, I_IndirectArgumentResolver>
			void AddIndirectArgument(T&& argumentResolver);

			Brawler::FileWriterNode CreateCommandSignatureDefinitionFileWriterNode() const;

		private:
			std::vector<std::unique_ptr<I_IndirectArgumentResolver>> mArgResolverPtrArr;
		};
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace CommandSignatures
	{
		template <CommandSignatureID CSIdentifier>
		template <typename T>
			requires std::derived_from<T, I_IndirectArgumentResolver>
		void CommandSignatureBuilder<CSIdentifier>::AddIndirectArgument(T&& argumentResolver)
		{
			mArgResolverPtrArr.push_back(std::make_unique<T>(std::move(argumentResolver)));
		}

		template <CommandSignatureID CSIdentifier>
		Brawler::FileWriterNode CommandSignatureBuilder<CSIdentifier>::CreateCommandSignatureDefinitionFileWriterNode() const
		{
			assert(!mArgResolverPtrArr.empty() && "ERROR: An attempt was made to use a CommandSignatureBuilder instance without assigning any I_IndirectArgumentResolver instances to it!");
			
			static constexpr std::string_view COMMAND_SIGNATURE_ID_STRING{ Brawler::GetCommandSignatureIDString<CSIdentifier>() };
			
			FileWriterNode rootNode{};

			{
				static constexpr std::string_view DEFINITION_BEGIN_FORMAT_STR{
R"(		template <>
		struct CommandSignatureDefinition<CommandSignatureID::{}>
		{{
		private:
)"
				};

				FileWriterNode definitionBeginNode{};
				definitionBeginNode.SetOutputText(std::format(DEFINITION_BEGIN_FORMAT_STR, COMMAND_SIGNATURE_ID_STRING));

				rootNode.AddChildNode(std::move(definitionBeginNode));
			}

			// Add the nodes for the D3D12_INDIRECT_ARGUMENT_DESC to rootNode. Each I_IndirectArgumentResolver
			// instance in mArgResolverPtrArr describes an indirect argument which is to be added to the
			// command signature. The combined set of indirect arguments forms the command sent in a call to
			// ID3D12GraphicsCommandList::ExecuteIndirect().
			{
				FileWriterNode indirectArgumentDescriptionsHeadNode{};

				// The actual D3D12_INDIRECT_ARGUMENT_DESC instances won't be exposed to other classes, as they
				// will be private members of the CommandSignatureDefinition instance. It doesn't really matter
				// what we name these, so we'll just call them INDIRECT_ARGUMENT_X, where X is the zero-based
				// index of the indirect argument.

				for (const auto i : std::views::iota(0ull, mArgResolverPtrArr.size()))
				{
					static constexpr std::string_view INDIRECT_ARGUMENT_DESC_DEFINITION_FORMAT_STR{
R"(			static constexpr D3D12_INDIRECT_ARGUMENT_DESC INDIRECT_ARGUMENT_{}{{
{}
			}};

)"
					};

					FileWriterNode currDescDefinitionNode{};
					currDescDefinitionNode.SetOutputText(std::format(INDIRECT_ARGUMENT_DESC_DEFINITION_FORMAT_STR, i, mArgResolverPtrArr[i]->CreateIndirectArgumentDescriptionInitializerString()));

					indirectArgumentDescriptionsHeadNode.AddChildNode(std::move(currDescDefinitionNode));
				}

				{
					static constexpr std::string_view INDIRECT_ARGUMENT_DESC_ARR_FORMAT_STR{
R"(			static constexpr std::array<D3D12_INDIRECT_ARGUMENT_DESC, {}> INDIRECT_ARGUMENT_DESCRIPTION_ARR{{
{}
			}};

)"
					};

					std::string argNameListStr{ "\t\t\t\tINDIRECT_ARGUMENT_0" };

					for (const auto i : std::views::iota(1ull, mArgResolverPtrArr.size()))
						argNameListStr += std::format(",\n\t\t\t\tINDIRECT_ARGUMENT_{}", i);

					FileWriterNode argDescArrNode{};
					argDescArrNode.SetOutputText(std::format(INDIRECT_ARGUMENT_DESC_ARR_FORMAT_STR, mArgResolverPtrArr.size(), argNameListStr));

					indirectArgumentDescriptionsHeadNode.AddChildNode(std::move(argDescArrNode));
				}

				rootNode.AddChildNode(std::move(indirectArgumentDescriptionsHeadNode));
			}

			{
				static constexpr std::string_view INDIRECT_ARGUMENTS_LAYOUT_FORMAT_STR{
R"(		private:
			// Indirect arguments are tightly packed. This is specified directly in the MSDN:
			//
			// "The ordering of arguments within an indirect argument buffer is defined to exactly match the
			// order of arguments specified in the pArguments parameter of D3D12_COMMAND_SIGNATURE_DESC. All
			// of the arguments for one draw (graphics)/dispatch (compute) call within an indirect argument
			// buffer are tightly packed. However, applications are allowed to specify an arbitrary byte
			// stride between draw/dispatch commands in an indirect argument buffer."
			//
			// (Source: https://learn.microsoft.com/en-us/windows/win32/direct3d12/indirect-drawing#command-signature-creation)

#pragma pack(push)
#pragma pack(1)
{}
#pragma pack(pop)

)"
				};

				// Add any custom type definitions specified by the I_IndirectArgumentResolver instances.
				std::string customTypeDefinitionsStr{};

				for (const auto& argResolverPtr : mArgResolverPtrArr)
				{
					const std::optional<std::string_view> currCustomTypeDefinitionStr{ argResolverPtr->GetCustomTypeDefinitionString() };

					if (currCustomTypeDefinitionStr.has_value())
						customTypeDefinitionsStr += std::string{ *currCustomTypeDefinitionStr } + "\n\n";
				}

				static constexpr std::string_view INDIRECT_ARGUMENTS_LAYOUT_DEFINITION_FORMAT_STR{
R"(			struct IndirectArgumentsLayout
			{{
{}
			}};)"
				};

				std::string layoutFieldsStr{ std::format("\t\t\t\t{} {};", mArgResolverPtrArr.front()->GetIndirectArgumentFieldTypeString(), mArgResolverPtrArr.front()->GetIndirectArgumentFieldNameString()) };

				for (const auto& argResolverPtr : mArgResolverPtrArr | std::views::drop(1))
					layoutFieldsStr += std::format("\n\t\t\t\t{} {};", argResolverPtr->GetIndirectArgumentFieldTypeString(), argResolverPtr->GetIndirectArgumentFieldNameString());

				const std::string indirectArgumentsLayoutDefinitionStr{ std::format(INDIRECT_ARGUMENTS_LAYOUT_DEFINITION_FORMAT_STR, layoutFieldsStr) };

				FileWriterNode indirectArgumentsLayoutNode{};
				indirectArgumentsLayoutNode.SetOutputText(std::format(INDIRECT_ARGUMENTS_LAYOUT_FORMAT_STR, std::string{ customTypeDefinitionsStr + indirectArgumentsLayoutDefinitionStr }));

				rootNode.AddChildNode(std::move(indirectArgumentsLayoutNode));
			}

			{
				static constexpr std::string_view DEFINITION_END_FORMAT_STR{
R"(		public:
			using CommandSignatureType = IndirectArgumentsLayout;

		public:
			static constexpr D3D12_COMMAND_SIGNATURE_DESC COMMAND_SIGNATURE_DESCRIPTION{{
				.ByteStride = sizeof(CommandSignatureType),
				.NumArgumentDescs = static_cast<std::uint32_t>(INDIRECT_ARGUMENT_DESCRIPTION_ARR.size()),
				.pArgumentDescs = INDIRECT_ARGUMENT_DESCRIPTION_ARR.data()
			}};

{}
		}};
)"
				};

				static constexpr std::optional<RootSignatureID> ASSOCIATED_ROOT_SIGNATURE_ID{ Brawler::GetRootSignatureForCommandSignature<CSIdentifier>() };

				if constexpr (ASSOCIATED_ROOT_SIGNATURE_ID.has_value())
				{
					static constexpr std::string_view ROOT_SIGNATURE_ID_FORMAT_STR{
R"(			// The MSVC really doesn't like it when static constexpr std::optional instances are used in C++20
			// modules. Instead, we use RootSignatures::RootSignatureID::COUNT_OR_ERROR to represent a command
			// signature which does not have an associated root signature.
			static constexpr RootSignatures::RootSignatureID ASSOCIATED_ROOT_SIGNATURE_ID = RootSignatures::RootSignatureID::{};)"
					};

					std::string rootSignatureIDStr{ std::format(ROOT_SIGNATURE_ID_FORMAT_STR, Brawler::GetRootSignatureIDString<*ASSOCIATED_ROOT_SIGNATURE_ID>()) };

					FileWriterNode definitionEndNode{};
					definitionEndNode.SetOutputText(std::format(DEFINITION_END_FORMAT_STR, rootSignatureIDStr));

					rootNode.AddChildNode(std::move(definitionEndNode));
				}
				else
				{
					static constexpr std::string_view EMPTY_ROOT_SIGNATURE_ID_STR{
R"(			// The MSVC really doesn't like it when static constexpr std::optional instances are used in C++20
			// modules. Instead, we use RootSignatures::RootSignatureID::COUNT_OR_ERROR to represent a command
			// signature which does not have an associated root signature.
			static constexpr RootSignatures::RootSignatureID ASSOCIATED_ROOT_SIGNATURE_ID = RootSignatures::RootSignatureID::COUNT_OR_ERROR;)"
					};

					FileWriterNode definitionEndNode{};
					definitionEndNode.SetOutputText(std::format(DEFINITION_END_FORMAT_STR, EMPTY_ROOT_SIGNATURE_ID_STR));

					rootNode.AddChildNode(std::move(definitionEndNode));
				}
			}

			return rootNode;
		}
	}
}