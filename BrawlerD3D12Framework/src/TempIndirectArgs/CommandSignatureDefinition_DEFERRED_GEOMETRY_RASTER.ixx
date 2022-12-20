module;
#include <array>
#include <optional>
#include "../DxDef.h"

export module Brawler.CommandSignatures.CommandSignatureDefinition:CommandSignatureDefinition_DEFERRED_GEOMETRY_RASTER;
import :CommandSignatureDefinitionBase;
import Brawler.CommandSignatures.CommandSignatureID;
import Brawler.RootSignatures.RootSignatureID;

/*
Implementation Details:

  - The MSDN is outdated in regards to listing all of the indirect argument buffer structures. Here is the complete
    listing corresponding to each D3D12_INDIRECT_ARGUMENT_TYPE value (unless otherwise stated, the source for this
	information is https://learn.microsoft.com/en-us/windows/win32/direct3d12/indirect-drawing#indirect-argument-buffer-structures):

	- D3D12_INDIRECT_ARGUMENT_TYPE_DRAW -> D3D12_DRAW_ARGUMENTS
	- D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED -> D3D12_DRAW_INDEXED_ARGUMENTS
	- D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH -> D3D12_DISPATCH_ARGUMENTS
	- D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW -> D3D12_VERTEX_BUFFER_VIEW
	- D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW -> D3D12_INDEX_BUFFER_VIEW

	- D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT -> [This can be any struct whose size is a multiple of sizeof(std::uint32_t), but its size
	                                            must match that specified in the corresponding D3D12_INDIRECT_ARGUMENT_DESC instance.]

	- D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW -> D3D12_GPU_VIRTUAL_ADDRESS
	- D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW -> D3D12_GPU_VIRTUAL_ADDRESS
	- D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW -> D3D12_GPU_VIRTUAL_ADDRESS
	
	- D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS -> D3D12_DISPATCH_RAYS_DESC (Source: https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#d3d12_indirect_argument_type)

	- D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH -> D3D12_DISPATCH_MESH_ARGUMENTS (Source: https://microsoft.github.io/DirectX-Specs/d3d/MeshShader.html#executeindirect)

  - The ExecuteIndirect API offers no methods for setting descriptor table root parameters (only root constants and
    root CBVs/SRVs/UAVs).
*/

export namespace Brawler
{
	namespace CommandSignatures
	{
		template <>
		struct CommandSignatureDefinition<CommandSignatureID::DEFERRED_GEOMETRY_RASTER>
		{
		private:
			static constexpr D3D12_INDIRECT_ARGUMENT_DESC INDIRECT_ARGUMENT_0{
				.Type = D3D12_INDIRECT_ARGUMENT_TYPE::D3D12_INDIRECT_ARGUMENT_TYPE_DRAW
			};

			static constexpr std::array<D3D12_INDIRECT_ARGUMENT_DESC, 1> INDIRECT_ARGUMENT_DESCRIPTION_ARR{
				INDIRECT_ARGUMENT_0
			};

		private:
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
			struct IndirectArgumentsLayout
			{
				D3D12_DRAW_ARGUMENTS Param0;
			};
#pragma pack(pop)

		public:
			using IndirectArgumentsType = IndirectArgumentsLayout;

		public:
			static constexpr D3D12_COMMAND_SIGNATURE_DESC COMMAND_SIGNATURE_DESCRIPTION{
				.ByteStride = sizeof(IndirectArgumentsType),
				.NumArgumentDescs = static_cast<std::uint32_t>(INDIRECT_ARGUMENT_DESCRIPTION_ARR.size()),
				.pArgumentDescs = INDIRECT_ARGUMENT_DESCRIPTION_ARR.data()
			};

			static constexpr std::optional<RootSignatures::RootSignatureID> ASSOCIATED_ROOT_SIGNATURE_ID{};
		};
	}
}