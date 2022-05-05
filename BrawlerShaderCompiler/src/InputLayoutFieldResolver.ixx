module;
#include <string>
#include <vector>
#include "DxDef.h"

export module Brawler.InputLayoutFieldResolver;
import Brawler.I_PSOFieldResolver;

export namespace Brawler
{
	class InputLayoutFieldResolver final : public I_PSOFieldResolver
	{
	public:
		InputLayoutFieldResolver() = default;

		/// <summary>
		/// Adds a per-vertex input element to the input layout used by the associated
		/// vertex shader. The element is added directly after the previous one.
		/// 
		/// NOTE: The element is sent through input slot 0. This enforces that only one vertex
		/// buffer is bound to the pipeline at a time.
		/// </summary>
		/// <param name="semanticName">
		/// - The semantic name of this input element. HLSL uses this value to correctly initialize
		///   the structure passed to the vertex shader's entry point.
		/// </param>
		/// <param name="format">
		/// - The format of the data represented by this input element.
		/// </param>
		void AddPerVertexInputElement(const std::string_view semanticName, const DXGI_FORMAT format);

		/// <summary>
		/// Adds a per-instance input element to the input layout used by the associated
		/// vertex shader. The element is added directly after the previous one.
		/// 
		/// NOTE: The element is sent through input slot 0. This enforces that only one vertex
		/// buffer is bound to the pipeline at a time.
		/// </summary>
		/// <param name="semanticName">
		/// - The semantic name of this input element. HLSL uses this value to correctly initialize
		///   the structure passed to the vertex shader's entry point.
		/// </param>
		/// <param name="format">
		/// - The format of the data represented by this input element.
		/// </param>
		/// <param name="instanceDataStepRate">
		/// - The number of instances to draw using the same per-instance data before advancing
		///   in the buffer by one element.
		/// </param>
		void AddPerInstanceInputElement(const std::string_view semanticName, const DXGI_FORMAT format, const std::uint32_t instanceDataStepRate);

		FileWriterNode CreatePSODefinitionFileWriterNode() const override;
		FileWriterNode CreateRuntimePSOResolutionFileWriterNode() const override;

	private:
		bool IsSemanticNameClaimed(const std::string_view semanticName) const;

	private:
		std::vector<D3D12_INPUT_ELEMENT_DESC> mElementDescArr;
	};
}