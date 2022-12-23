module;
#include <string>
#include <vector>
#include <ranges>
#include <optional>
#include <array>
#include "DxDef.h"

module Brawler.InputLayoutFieldResolver;
import Brawler.PSOID;
import Brawler.PSOField;
import Brawler.PSODefinition;

namespace
{
	std::string CreateInputElementInitializerListString(const D3D12_INPUT_ELEMENT_DESC& elementDesc)
	{
		std::string inputElementStr{ "D3D12_INPUT_ELEMENT_DESC{ \"" };
		inputElementStr += elementDesc.SemanticName;

		inputElementStr += "\", ";
		inputElementStr += std::to_string(elementDesc.SemanticIndex);

		// I mean, I *GUESS* I could create a function which returns a std::string_view for all 100+
		// DXGI_FORMAT values, but... really? Come on.
		inputElementStr += ", static_cast<DXGI_FORMAT>(";
		inputElementStr += std::to_string(std::to_underlying(elementDesc.Format));

		inputElementStr += "), ";
		inputElementStr += std::to_string(elementDesc.InputSlot);

		inputElementStr += ", ";

		// We'll make it explicitly clear that 0xFFFFFFFF is actually D3D12_APPEND_ALIGNED_ELEMENT.
		if (elementDesc.AlignedByteOffset == D3D12_APPEND_ALIGNED_ELEMENT) [[likely]]
			inputElementStr += "D3D12_APPEND_ALIGNED_ELEMENT";
		else
			inputElementStr += std::to_string(elementDesc.AlignedByteOffset);

		inputElementStr += ", D3D12_INPUT_CLASSIFICATION::";

		if (elementDesc.InputSlotClass == D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA)
			inputElementStr += "D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, ";
		else
			inputElementStr += "D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, ";

		inputElementStr += std::to_string(elementDesc.InstanceDataStepRate);
		inputElementStr += " }";

		return inputElementStr;
	}
}

namespace Brawler
{
	FileWriterNode InputLayoutFieldResolver::CreatePSODefinitionFileWriterNode() const
	{
		const std::vector<D3D12_INPUT_ELEMENT_DESC> elementDescArr{ GetD3D12InputElementDescriptionArray() };

		if (elementDescArr.empty()) [[unlikely]]
			return FileWriterNode{};

		std::string inputElementArrStr{ "\t\t\tstatic constexpr std::array<D3D12_INPUT_ELEMENT_DESC, " };
		inputElementArrStr += std::to_string(elementDescArr.size());
		inputElementArrStr += "> INPUT_ELEMENT_DESC_ARR{\n";

		inputElementArrStr += "\t\t\t\t";
		inputElementArrStr += CreateInputElementInitializerListString(elementDescArr[0]);

		for (const auto& elementDesc : elementDescArr | std::views::drop(1))
		{
			inputElementArrStr += ",\n\t\t\t\t";
			inputElementArrStr += CreateInputElementInitializerListString(elementDesc);
		}

		inputElementArrStr += "\n\t\t\t};\n\n";

		FileWriterNode inputElementArrNode{};
		inputElementArrNode.SetOutputText(std::move(inputElementArrStr));

		return inputElementArrNode;
	}

	FileWriterNode InputLayoutFieldResolver::CreateRuntimePSOResolutionFileWriterNode() const
	{
		std::string resolveStr{ "\t\t\t\tD3D12_INPUT_LAYOUT_DESC& InputLayout{ static_cast<D3D12_INPUT_LAYOUT_DESC&>(psoDesc." };
		resolveStr += Brawler::PSOField<CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT>::FIELD_NAME;
		resolveStr += ") };\n\t\t\t\tInputLayout.pInputElementDescs = INPUT_ELEMENT_DESC_ARR.data();\n\t\t\t\tInputLayout.NumElements = static_cast<std::uint32_t>(INPUT_ELEMENT_DESC_ARR.size());\n";

		FileWriterNode resolveNode{};
		resolveNode.SetOutputText(std::move(resolveStr));

		return resolveNode;
	}

	std::vector<D3D12_INPUT_ELEMENT_DESC> InputLayoutFieldResolver::GetD3D12InputElementDescriptionArray() const
	{
		std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescArr{};

		for (const auto inputSlotIndex : std::views::iota(0ull, mInputSlotDescArr.size()))
		{
			const std::optional<InputSlotDescription>& currInputSlotDesc{ mInputSlotDescArr[inputSlotIndex] };

			if (!currInputSlotDesc.has_value()) [[likely]]
				continue;
			
			for (const auto& inputElementDesc : currInputSlotDesc->InputElementDescArr)
			{
				inputElementDescArr.push_back(D3D12_INPUT_ELEMENT_DESC{
					.SemanticName = inputElementDesc.SemanticName.data(),
					.Format = inputElementDesc.Format,
					.InputSlot = static_cast<std::uint32_t>(inputSlotIndex),
					.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
					.InputSlotClass = currInputSlotDesc->InputSlotClass,
					.InstanceDataStepRate = currInputSlotDesc->InstanceDataStepRate
				});
			}
		}

		return inputElementDescArr;
	}

	bool InputLayoutFieldResolver::IsSemanticNameClaimed(const std::string_view semanticName) const
	{
		for (const auto& inputSlotDesc : mInputSlotDescArr | std::views::filter([] (const std::optional<InputSlotDescription>& inputSlotDesc) { return inputSlotDesc.has_value(); }))
		{
			for (const auto& inputElementDesc : inputSlotDesc->InputElementDescArr)
			{
				if (inputElementDesc.SemanticName == semanticName)
					return true;
			}
		}

		return false;
	}
}