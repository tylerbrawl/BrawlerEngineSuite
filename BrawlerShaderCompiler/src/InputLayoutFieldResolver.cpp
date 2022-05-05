module;
#include <string>
#include <vector>
#include <ranges>
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
	void InputLayoutFieldResolver::AddPerVertexInputElement(const std::string_view semanticName, const DXGI_FORMAT format)
	{
		// First, make sure that we do not have any naming conflicts.
		if (IsSemanticNameClaimed(semanticName)) [[unlikely]]
			throw std::runtime_error{ "ERROR: A semantic name conflict was detected between the input elements of an InputLayoutFieldResolver!" };

		mElementDescArr.push_back(D3D12_INPUT_ELEMENT_DESC{
			.SemanticName = semanticName.data(),
			.SemanticIndex = 0,
			.Format = format,
			.InputSlot = 0,
			.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
			.InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			.InstanceDataStepRate = 0
		});
	}

	void InputLayoutFieldResolver::AddPerInstanceInputElement(const std::string_view semanticName, const DXGI_FORMAT format, const std::uint32_t instanceDataStepRate)
	{
		// First, make sure that we do not have any naming conflicts.
		if (IsSemanticNameClaimed(semanticName)) [[unlikely]]
			throw std::runtime_error{ "ERROR: A semantic name conflict was detected between the input elements of an InputLayoutFieldResolver!" };

		mElementDescArr.push_back(D3D12_INPUT_ELEMENT_DESC{
			.SemanticName = semanticName.data(),
			.SemanticIndex = 0,
			.Format = format,
			.InputSlot = 0,
			.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
			.InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,
			.InstanceDataStepRate = instanceDataStepRate
		});
	}

	FileWriterNode InputLayoutFieldResolver::CreatePSODefinitionFileWriterNode() const
	{
		if (mElementDescArr.empty()) [[unlikely]]
			return FileWriterNode{};

		std::string inputElementArrStr{ "\t\t\tstatic constexpr std::array<D3D12_INPUT_ELEMENT_DESC, " };
		inputElementArrStr += std::to_string(mElementDescArr.size());
		inputElementArrStr += "> INPUT_ELEMENT_DESC_ARR{\n";

		inputElementArrStr += "\t\t\t\t";
		inputElementArrStr += CreateInputElementInitializerListString(mElementDescArr[0]);

		for (const auto& elementDesc : mElementDescArr | std::views::drop(1))
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

	bool InputLayoutFieldResolver::IsSemanticNameClaimed(const std::string_view semanticName) const
	{
		for (const auto& inputElement : mElementDescArr)
		{
			if (inputElement.SemanticName == semanticName) [[unlikely]]
				return true;
		}

		return false;
	}
}