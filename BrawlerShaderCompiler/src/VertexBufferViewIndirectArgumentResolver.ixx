module;
#include <string>
#include <string_view>
#include <optional>
#include <cstdint>

export module Brawler.IndirectArgumentResolvers:VertexBufferViewIndirectArgumentResolver;
import :I_IndirectArgumentResolver;

export namespace Brawler
{
	class VertexBufferViewIndirectArgumentResolver final : public I_IndirectArgumentResolver
	{
	public:
		explicit VertexBufferViewIndirectArgumentResolver(std::string argumentNameStr);

		VertexBufferViewIndirectArgumentResolver(const VertexBufferViewIndirectArgumentResolver& rhs) = delete;
		VertexBufferViewIndirectArgumentResolver& operator=(const VertexBufferViewIndirectArgumentResolver& rhs) = delete;

		VertexBufferViewIndirectArgumentResolver(VertexBufferViewIndirectArgumentResolver&& rhs) noexcept = default;
		VertexBufferViewIndirectArgumentResolver& operator=(VertexBufferViewIndirectArgumentResolver&& rhs) noexcept = default;

		void SetVertexBufferSlot(const std::uint32_t vertexBufferSlot);

		std::string CreateIndirectArgumentDescriptionInitializerString() const override;
		std::string_view GetIndirectArgumentFieldTypeString() const override;
		std::string_view GetIndirectArgumentFieldNameString() const override;
		std::optional<std::string_view> GetCustomTypeDefinitionString() const override;

	private:
		std::string mArgumentNameStr;
		std::uint32_t mVertexBufferSlot;
	};
}