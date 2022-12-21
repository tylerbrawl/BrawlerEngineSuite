module;
#include <string>
#include <string_view>
#include <optional>
#include <cstdint>
#include <format>

module Brawler.IndirectArgumentResolvers;

namespace Brawler
{
	VertexBufferViewIndirectArgumentResolver::VertexBufferViewIndirectArgumentResolver(std::string argumentNameStr) :
		I_IndirectArgumentResolver(),
		mArgumentNameStr(std::move(argumentNameStr)),
		mVertexBufferSlot(0)
	{}

	void VertexBufferViewIndirectArgumentResolver::SetVertexBufferSlot(const std::uint32_t vertexBufferSlot)
	{
		mVertexBufferSlot = vertexBufferSlot;
	}

	std::string VertexBufferViewIndirectArgumentResolver::CreateIndirectArgumentDescriptionInitializerString() const
	{
		static constexpr std::string_view INITIALIZER_FORMAT_STRING{
R"(				.Type = D3D12_INDIRECT_ARGUMENT_TYPE::D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW,
				.VertexBuffer{{
					.Slot = {}
				}})"
		};

		return std::format(INITIALIZER_FORMAT_STRING, mVertexBufferSlot);
	}

	std::string_view VertexBufferViewIndirectArgumentResolver::GetIndirectArgumentFieldTypeString() const
	{
		static constexpr std::string_view FIELD_TYPE_STRING{ "D3D12_VERTEX_BUFFER_VIEW" };
		return FIELD_TYPE_STRING;
	}

	std::string_view VertexBufferViewIndirectArgumentResolver::GetIndirectArgumentFieldNameString() const
	{
		return std::string_view{ mArgumentNameStr };
	}

	std::optional<std::string_view> VertexBufferViewIndirectArgumentResolver::GetCustomTypeDefinitionString() const
	{
		return {};
	}
}