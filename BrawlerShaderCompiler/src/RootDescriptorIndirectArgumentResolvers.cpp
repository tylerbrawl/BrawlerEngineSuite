module;
#include <string>
#include <string_view>
#include <optional>
#include <cassert>
#include <format>

module Brawler.IndirectArgumentResolvers;

namespace
{
	template <Brawler::RootDescriptorID DescriptorID>
	struct RootDescriptorDefinition
	{
		static_assert(sizeof(DescriptorID) != sizeof(DescriptorID), "ERROR: An explicit template specialization of <anonymous namespace>::RootDescriptorDefinition was never provided for this Brawler::RootDescriptorID! (See RootDescriptorIndirectArgumentResolvers.cpp.)");
	};

	template <>
	struct RootDescriptorDefinition<Brawler::RootDescriptorID::CBV>
	{
		static constexpr std::string_view INITIALIZER_FORMAT_STRING{
R"(				.Type = D3D12_INDIRECT_ARGUMENT_TYPE::D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW,
				.ConstantBufferView{{
					.RootParameterIndex = {}
				}})"
		};
	};

	template <>
	struct RootDescriptorDefinition<Brawler::RootDescriptorID::SRV>
	{
		static constexpr std::string_view INITIALIZER_FORMAT_STRING{
R"(				.Type = D3D12_INDIRECT_ARGUMENT_TYPE::D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW,
				.ShaderResourceView{{
					.RootParameterIndex = {}
				}})"
		};
	};

	template <>
	struct RootDescriptorDefinition<Brawler::RootDescriptorID::UAV>
	{
		static constexpr std::string_view INITIALIZER_FORMAT_STRING{
R"(				.Type = D3D12_INDIRECT_ARGUMENT_TYPE::D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW,
				.UnorderedAccessView{{
					.RootParameterIndex = {}
				}})"
		};
	};
}

namespace Brawler
{
	template <RootDescriptorID DescriptorID>
	RootDescriptorIndirectArgumentResolver<DescriptorID>::RootDescriptorIndirectArgumentResolver(std::string argumentNameStr) :
		I_IndirectArgumentResolver(),
		mArgumentNameStr(std::move(argumentNameStr)),
		mRootParameterIndex()
	{}

	template <RootDescriptorID DescriptorID>
	void RootDescriptorIndirectArgumentResolver<DescriptorID>::SetRootParameterIndex(const std::uint32_t rootParameterIndex)
	{
		mRootParameterIndex = rootParameterIndex;
	}

	template <RootDescriptorID DescriptorID>
	std::string RootDescriptorIndirectArgumentResolver<DescriptorID>::CreateIndirectArgumentDescriptionInitializerString() const
	{
		assert(mRootParameterIndex.has_value() && "ERROR: A RootDescriptorIndirectArgumentResolver instance was never assigned a root parameter index! (This can be done by calling RootDescriptorIndirectArgumentResolver::SetRootParameterIndex().)");
		
		static constexpr std::string_view INITIALIZER_FORMAT_STRING{ RootDescriptorDefinition<DescriptorID>::INITIALIZER_FORMAT_STRING };

		return std::format(INITIALIZER_FORMAT_STRING, *mRootParameterIndex);
	}

	template <RootDescriptorID DescriptorID>
	std::string_view RootDescriptorIndirectArgumentResolver<DescriptorID>::GetIndirectArgumentFieldTypeString() const
	{
		static constexpr std::string_view FIELD_TYPE_STRING{ "D3D12_GPU_VIRTUAL_ADDRESS" };
		return FIELD_TYPE_STRING;
	}

	template <RootDescriptorID DescriptorID>
	std::string_view RootDescriptorIndirectArgumentResolver<DescriptorID>::GetIndirectArgumentFieldNameString() const
	{
		return std::string_view{ mArgumentNameStr };
	}

	template <RootDescriptorID DescriptorID>
	std::optional<std::string_view> RootDescriptorIndirectArgumentResolver<DescriptorID>::GetCustomTypeDefinitionString() const
	{
		return {};
	}
}

namespace Brawler
{
	template class RootDescriptorIndirectArgumentResolver<RootDescriptorID::CBV>;
	template class RootDescriptorIndirectArgumentResolver<RootDescriptorID::SRV>;
	template class RootDescriptorIndirectArgumentResolver<RootDescriptorID::UAV>;
}