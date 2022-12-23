module;
#include <array>
#include <string_view>
#include <vector>
#include <optional>
#include "DxDef.h"

export module Brawler.InputLayoutFieldResolver;
import Brawler.PerVertexIASlotDescription;
import Brawler.PerInstanceIASlotDescription;
import Brawler.I_PSOFieldResolver;

export namespace Brawler
{
	class InputLayoutFieldResolver final : public I_PSOFieldResolver
	{
	private:
		struct InputElementDescription
		{
			std::string_view SemanticName;
			DXGI_FORMAT Format;
		};

		struct InputSlotDescription
		{
			std::vector<InputElementDescription> InputElementDescArr;
			
			// Despite InputSlotClass and InstanceDataStepRate being members of the
			// D3D12_INPUT_ELEMENT_DESC struct, according to the D3D11 specifications,
			// they need to have the same value for all input elements assigned to a
			// given input assembler (IA) slot.

			D3D12_INPUT_CLASSIFICATION InputSlotClass;
			std::uint32_t InstanceDataStepRate;
		};

	private:
		// Valid values for the input slot are in the range [0, 15].
		//
		// (Source: https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_input_element_desc)
		static constexpr std::size_t INPUT_SLOT_COUNT = 16;

	public:
		InputLayoutFieldResolver() = default;

		InputLayoutFieldResolver(const InputLayoutFieldResolver& rhs) = delete;
		InputLayoutFieldResolver& operator=(const InputLayoutFieldResolver& rhs) = delete;

		InputLayoutFieldResolver(InputLayoutFieldResolver&& rhs) noexcept = default;
		InputLayoutFieldResolver& operator=(InputLayoutFieldResolver&& rhs) noexcept = default;

		/// <summary>
		/// Sets the data used for the input slot identified by InputSlot based on the input
		/// elements described within perVertexIASlotDescription.
		/// 
		/// Input slots are identified with zero-based indices. Since there are at most 16
		/// input slots, valid values for InputSlot are in the range [0, 15]. It is a
		/// compile-time error to specify a value outside of that range.
		/// </summary>
		/// <typeparam name="InputSlot">
		/// - The index of the input assembler (IA) slot which the specified PerVertexIASlotDescription
		///   instance describes. This value *MUST* be in the range [0, 15].
		/// </typeparam>
		/// <param name="perVertexIASlotDescription">
		/// - A const PerVertexIASlotDescription&amp; which describes the input elements assigned
		///   to the input assembler (IA) slot identified by InputSlot. As implied by the name of
		///   the parameter, all specified input elements are classified as
		///   D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA.
		/// </param>
		template <std::size_t InputSlot>
		void SetInputSlotData(const PerVertexIASlotDescription& perVertexIASlotDescription);

		/// <summary>
		/// Sets the data used for the input slot identified by InputSlot based on the input
		/// elements described within perInstanceIASlotDescription.
		/// 
		/// Input slots are identified with zero-based indices. Since there are at most 16
		/// input slots, valid values for InputSlot are in the range [0, 15]. It is a
		/// compile-time error to specify a value outside of that range.
		/// </summary>
		/// <typeparam name="InputSlot">
		/// - The index of the input assembler (IA) slot which the specified PerInstanceIASlotDescription
		///   instance describes. This value *MUST* be in the range [0, 15].
		/// </typeparam>
		/// <param name="perInstanceIASlotDescription">
		/// - A const PerInstanceIASlotDescription&amp; which describes the input elements assigned
		///   to the input assembler (IA) slot identified by InputSlot. As implied by the name of
		///   the parameter, all specified input elements are classified as
		///   D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA.
		template <std::size_t InputSlot>
		void SetInputSlotData(const PerInstanceIASlotDescription& perInstanceIASlotDescription);

		FileWriterNode CreatePSODefinitionFileWriterNode() const override;
		FileWriterNode CreateRuntimePSOResolutionFileWriterNode() const override;

	private:
		std::vector<D3D12_INPUT_ELEMENT_DESC> GetD3D12InputElementDescriptionArray() const;
		bool IsSemanticNameClaimed(const std::string_view semanticName) const;

	private:
		std::array<std::optional<InputSlotDescription>, INPUT_SLOT_COUNT> mInputSlotDescArr;
	};
}

// --------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <std::size_t InputSlot>
	void InputLayoutFieldResolver::SetInputSlotData(const PerVertexIASlotDescription& perVertexIASlotDescription)
	{
		static_assert(InputSlot < INPUT_SLOT_COUNT, "ERROR: An out-of-bounds input assembler (IA) slot identifier was specified in a call to InputLayoutFieldResolver::SetInputSlotData()! (The IA supports at most 16 input slots, so valid values are in the range [0, 15].");

		mInputSlotDescArr[InputSlot].reset();
		
		const auto inputElementSpan{ perVertexIASlotDescription.GetInputElementSpan() };
		assert(!inputElementSpan.empty() && "ERROR: An attempt was made to describe the input elements for an input assembler (IA) slot in a call to InputLayoutFieldResolver::SetInputSlotData(), but the input element list was empty!");

		std::vector<InputElementDescription> inputElementDescArr{};
		inputElementDescArr.reserve(inputElementSpan.size());

		for (const auto& inputElement : inputElementSpan)
		{
			assert(!IsSemanticNameClaimed(inputElement.SemanticName) && "ERROR: An attempt was made to specify more than one input element with the same semantic name in a given IA input layout!");
			
			inputElementDescArr.push_back(InputElementDescription{
				.SemanticName{ inputElement.SemanticName },
				.Format = inputElement.Format
			});
		}

		mInputSlotDescArr[InputSlot] = InputSlotDescription{
			.InputElementDescArr{ std::move(inputElementDescArr) },
			.InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,

			// According to the D3D11 specifications, InstanceDataStepRate must be 0 when
			// the input classification is set to D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA.
			.InstanceDataStepRate = 0
		};
	}

	template <std::size_t InputSlot>
	void InputLayoutFieldResolver::SetInputSlotData(const PerInstanceIASlotDescription& perInstanceIASlotDescription)
	{
		static_assert(InputSlot < INPUT_SLOT_COUNT, "ERROR: An out-of-bounds input assembler (IA) slot identifier was specified in a call to InputLayoutFieldResolver::SetInputSlotData()! (The IA supports at most 16 input slots, so valid values are in the range [0, 15].");

		mInputSlotDescArr[InputSlot].reset();
		
		const auto inputElementSpan{ perInstanceIASlotDescription.GetInputElementSpan() };
		assert(!inputElementSpan.empty() && "ERROR: An attempt was made to describe the input elements for an input assembler (IA) slot in a call to InputLayoutFieldResolver::SetInputSlotData(), but the input element list was empty!");

		std::vector<InputElementDescription> inputElementDescArr{};
		inputElementDescArr.reserve(inputElementSpan.size());

		for (const auto& inputElement : inputElementSpan)
		{
			assert(!IsSemanticNameClaimed(inputElement.SemanticName) && "ERROR: An attempt was made to specify more than one input element with the same semantic name in a given IA input layout!");
			
			inputElementDescArr.push_back(InputElementDescription{
				.SemanticName{ inputElement.SemanticName },
				.Format = inputElement.Format
			});
		}

		mInputSlotDescArr[InputSlot] = InputSlotDescription{
			.InputElementDescArr{ std::move(inputElementDescArr) },
			.InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,
			.InstanceDataStepRate = perInstanceIASlotDescription.GetInstanceDataStepRate()
		};
	}
}