module;
#include <vector>
#include <span>
#include <string_view>
#include "DxDef.h"

export module Brawler.PerVertexIASlotDescription;

export namespace Brawler
{
	class PerVertexIASlotDescription
	{
	private:
		struct PerVertexInputElement
		{
			std::string_view SemanticName;
			DXGI_FORMAT Format;
		};

	public:
		PerVertexIASlotDescription() = default;

		PerVertexIASlotDescription(const PerVertexIASlotDescription& rhs) = delete;
		PerVertexIASlotDescription& operator=(const PerVertexIASlotDescription& rhs) = delete;

		PerVertexIASlotDescription(PerVertexIASlotDescription&& rhs) noexcept = default;
		PerVertexIASlotDescription& operator=(PerVertexIASlotDescription&& rhs) noexcept = default;

		/// <summary>
		/// Adds a per-vertex input element to the input layout for this input assembler (IA) slot.
		/// The element is added immediately after the previous input element, as if the
		/// AlignedByteOffset field of the D3D12_INPUT_ELEMENT_DESC class were set to
		/// D3D12_APPEND_ALIGNED_ELEMENT.
		/// </summary>
		/// <param name="semanticName">
		/// - The semantic name of this input element. HLSL uses this value to correctly initialize
		///   the structure passed to the vertex shader's entry point.
		/// </param>
		/// <param name="format">
		/// - The format of the data represented by this input element.
		/// </param>
		void AddPerVertexInputElement(const std::string_view semanticName, const DXGI_FORMAT format);

		std::span<const PerVertexInputElement> GetInputElementSpan() const;

	private:
		std::vector<PerVertexInputElement> mInputElementArr;
	};
}