module;
#include <optional>
#include <string>
#include <string_view>

export module Brawler.IndirectArgumentResolvers:I_IndirectArgumentResolver;

export namespace Brawler
{
	class I_IndirectArgumentResolver
	{
	protected:
		I_IndirectArgumentResolver() = default;

	public:
		virtual ~I_IndirectArgumentResolver() = default;

		I_IndirectArgumentResolver(const I_IndirectArgumentResolver& rhs) = default;
		I_IndirectArgumentResolver& operator=(const I_IndirectArgumentResolver& rhs) = default;

		I_IndirectArgumentResolver(I_IndirectArgumentResolver&& rhs) noexcept = default;
		I_IndirectArgumentResolver& operator=(I_IndirectArgumentResolver&& rhs) noexcept = default;

		virtual std::string CreateIndirectArgumentDescriptionInitializerString() const = 0;

		virtual std::string_view GetIndirectArgumentFieldTypeString() const = 0;
		virtual std::string_view GetIndirectArgumentFieldNameString() const = 0;

		virtual std::optional<std::string_view> GetCustomTypeDefinitionString() const = 0;
	};
}