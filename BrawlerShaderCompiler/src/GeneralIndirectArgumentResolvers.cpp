module;
#include <optional>
#include <string>
#include <string_view>
#include "DxDef.h"

module Brawler.IndirectArgumentResolvers;

/*
The MSDN is outdated in regards to listing all of the indirect argument buffer structures. Here is the complete
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
*/

namespace
{
	template <Brawler::GeneralResolverID ResolverID>
	struct GeneralResolverDefinition
	{
		static_assert(sizeof(ResolverID) != sizeof(ResolverID), "ERROR: An explicit template specialization of <anonymous namespace>::GeneralResolverDefinition was never provided for this Brawler::GeneralResolverID! (See GeneralIndirectArgumentResolvers.cpp.)");
	};

	template <>
	struct GeneralResolverDefinition<Brawler::GeneralResolverID::DRAW>
	{
		static constexpr std::string_view FIELD_TYPE_STRING{ "D3D12_DRAW_ARGUMENTS" };
		static constexpr std::string_view ARGUMENT_DESC_INITIALIZER_STRING{ "\t\t\t\t.Type = D3D12_INDIRECT_ARGUMENT_TYPE::D3D12_INDIRECT_ARGUMENT_TYPE_DRAW" };
	};

	template <>
	struct GeneralResolverDefinition<Brawler::GeneralResolverID::DRAW_INDEXED>
	{
		static constexpr std::string_view FIELD_TYPE_STRING{ "D3D12_DRAW_INDEXED_ARGUMENTS" };
		static constexpr std::string_view ARGUMENT_DESC_INITIALIZER_STRING{ "\t\t\t\t.Type = D3D12_INDIRECT_ARGUMENT_TYPE::D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED" };
	};

	template <>
	struct GeneralResolverDefinition<Brawler::GeneralResolverID::DISPATCH>
	{
		static constexpr std::string_view FIELD_TYPE_STRING{ "D3D12_DISPATCH_ARGUMENTS" };
		static constexpr std::string_view ARGUMENT_DESC_INITIALIZER_STRING{ "\t\t\t\t.Type = D3D12_INDIRECT_ARGUMENT_TYPE::D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH" };
	};

	template <>
	struct GeneralResolverDefinition<Brawler::GeneralResolverID::INDEX_BUFFER_VIEW>
	{
		static constexpr std::string_view FIELD_TYPE_STRING{ "D3D12_INDEX_BUFFER_VIEW" };
		static constexpr std::string_view ARGUMENT_DESC_INITIALIZER_STRING{ "\t\t\t\t.Type = D3D12_INDIRECT_ARGUMENT_TYPE::D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW" };
	};

	template <>
	struct GeneralResolverDefinition<Brawler::GeneralResolverID::DISPATCH_RAYS>
	{
		static constexpr std::string_view FIELD_TYPE_STRING{ "D3D12_DISPATCH_RAYS_DESC" };
		static constexpr std::string_view ARGUMENT_DESC_INITIALIZER_STRING{ "\t\t\t\t.Type = D3D12_INDIRECT_ARGUMENT_TYPE::D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS" };
	};

	template <>
	struct GeneralResolverDefinition<Brawler::GeneralResolverID::DISPATCH_MESH>
	{
		static constexpr std::string_view FIELD_TYPE_STRING{ "D3D12_DISPATCH_MESH_ARGUMENTS" };
		static constexpr std::string_view ARGUMENT_DESC_INITIALIZER_STRING{ "\t\t\t\t.Type = D3D12_INDIRECT_ARGUMENT_TYPE::D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH" };
	};
}

namespace Brawler
{
	template <GeneralResolverID ResolverID>
	GeneralIndirectArgumentResolver<ResolverID>::GeneralIndirectArgumentResolver(std::string argumentNameStr) :
		I_IndirectArgumentResolver(),
		mArgumentNameStr(std::move(argumentNameStr))
	{}

	template <GeneralResolverID ResolverID>
	std::string GeneralIndirectArgumentResolver<ResolverID>::CreateIndirectArgumentDescriptionInitializerString() const
	{
		return std::string{ GeneralResolverDefinition<ResolverID>::ARGUMENT_DESC_INITIALIZER_STRING };
	}

	template <GeneralResolverID ResolverID>
	std::string_view GeneralIndirectArgumentResolver<ResolverID>::GetIndirectArgumentFieldTypeString() const
	{
		return GeneralResolverDefinition<ResolverID>::FIELD_TYPE_STRING;
	}

	template <GeneralResolverID ResolverID>
	std::string_view GeneralIndirectArgumentResolver<ResolverID>::GetIndirectArgumentFieldNameString() const
	{
		return std::string_view{ mArgumentNameStr };
	}

	template <GeneralResolverID ResolverID>
	std::optional<std::string_view> GeneralIndirectArgumentResolver<ResolverID>::GetCustomTypeDefinitionString() const
	{
		return {};
	}
}

namespace Brawler
{
	template class GeneralIndirectArgumentResolver<GeneralResolverID::DRAW>;
	template class GeneralIndirectArgumentResolver<GeneralResolverID::DRAW_INDEXED>;
	template class GeneralIndirectArgumentResolver<GeneralResolverID::DISPATCH>;
	template class GeneralIndirectArgumentResolver<GeneralResolverID::INDEX_BUFFER_VIEW>;
	template class GeneralIndirectArgumentResolver<GeneralResolverID::DISPATCH_RAYS>;
	template class GeneralIndirectArgumentResolver<GeneralResolverID::DISPATCH_MESH>;
}