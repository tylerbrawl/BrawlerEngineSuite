module;
#include <vector>
#include <span>
#include <string_view>
#include "DxDef.h"

export module Brawler.PerInstanceIASlotDescription;

export namespace Brawler
{
	class PerInstanceIASlotDescription
	{
	private:
		struct PerInstanceInputElement
		{
			std::string_view SemanticName;
			DXGI_FORMAT Format;
		};

	public:
		explicit PerInstanceIASlotDescription(const std::uint32_t instanceDataStepRate = 1);

		PerInstanceIASlotDescription(const PerInstanceIASlotDescription& rhs) = delete;
		PerInstanceIASlotDescription& operator=(const PerInstanceIASlotDescription& rhs) = delete;

		PerInstanceIASlotDescription(PerInstanceIASlotDescription&& rhs) noexcept = default;
		PerInstanceIASlotDescription& operator=(PerInstanceIASlotDescription&& rhs) noexcept = default;

		/// <summary>
		/// Adds a per-instance input element to the input layout for this input assembler (IA) slot.
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
		void AddPerInstanceInputElement(const std::string_view semanticName, const DXGI_FORMAT format);

		/// <summary>
		/// Sets the instance data step rate for input elements for this input assembler (IA) slot.
		/// 
		/// The instance data step rate determines how many instances are drawn before moving on
		/// to the next element in the vertex buffer assigned to this IA slot during vertex shader
		/// execution. For instance, if the instance data step rate is set to 1, then every instance
		/// drawn will get a different element in the vertex buffer.
		/// 
		/// If the instance data step rate is 0, then according to the D3D11 specifications, the
		/// same vertex buffer element is used for every instance drawn in a single call to
		/// DrawInstanced() or DrawIndexedInstanced(). The input element then essentially becomes
		/// "per-draw-call" data.
		/// 
		/// By default, the instance data step rate is set to 1.
		/// 
		/// (NOTE: According to the D3D11 specifications, the instance data step rate must be the same
		/// for all input elements for a given IA slot. This is why there is no API in this class to
		/// set it on a per-input-element basis, even though InstanceDataStepRate is a field of the
		/// D3D12_INPUT_ELEMENT_DESC struct.)
		/// </summary>
		/// <param name="instanceDataStepRate">
		/// - The number of instances to draw before moving on to the next element in the vertex
		///   buffer assigned to this IA slot. A value of 0 means to never move past the first
		///   element in the vertex buffer. See the function summary for more details.
		/// </param>
		void SetInstanceDataStepRate(const std::uint32_t instanceDataStepRate);

		/// <summary>
		/// Retrieves the instance data step rate for input elements for this input assembler (IA)
		/// slot.
		/// 
		/// The instance data step rate determines how many instances are drawn before moving on
		/// to the next element in the vertex buffer assigned to this IA slot during vertex shader
		/// execution. For instance, if the instance data step rate is set to 1, then every instance
		/// drawn will get a different element in the vertex buffer.
		/// 
		/// If the instance data step rate is 0, then according to the D3D11 specifications, the
		/// same vertex buffer element is used for every instance drawn in a single call to
		/// DrawInstanced() or DrawIndexedInstanced(). The input element then essentially becomes
		/// "per-draw-call" data.
		/// 
		/// By default, the instance data step rate is set to 1.
		/// 
		/// (NOTE: According to the D3D11 specifications, the instance data step rate must be the same
		/// for all input elements for a given IA slot. This is why there is no API in this class to
		/// set it on a per-input-element basis, even though InstanceDataStepRate is a field of the
		/// D3D12_INPUT_ELEMENT_DESC struct.)
		/// </summary>
		/// <returns>
		/// The function returns the instance data step rate for input elements for this IA slot.
		/// See the function summary for more details.
		/// </returns>
		std::uint32_t GetInstanceDataStepRate() const;

		std::span<const PerInstanceInputElement> GetInputElementSpan() const;

	private:
		std::vector<PerInstanceInputElement> mInputElementArr;
		std::uint32_t mInstanceDataStepRate;
	};
}